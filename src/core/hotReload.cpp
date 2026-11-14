#ifdef _WIN32
#include <winsock2.h> // windows.h 보다 먼저
#include <ws2tcpip.h>
#endif
#include "core/hotReload.h"
#include "core/App.h"
#include "core/log.h"
#include "core/luaScript.h"
#include <string>

#ifdef _WIN32
using sock_t = SOCKET;
static const sock_t BAD_SOCK = INVALID_SOCKET;
#define closesock closesocket
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
using sock_t = int;
static const sock_t BAD_SOCK = -1;
#define closesock close
#endif

static sock_t toSock(std::intptr_t v) { return (sock_t)v; }

static void setNonBlocking(sock_t fd, bool nb) {
#ifdef _WIN32
  u_long v = nb ? 1 : 0;
  ioctlsocket(fd, FIONBIO, &v);
#else
  int fl = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, nb ? (fl | O_NONBLOCK) : (fl & ~O_NONBLOCK));
#endif
}

bool hotReloadOpen(App &a, int port) {
#ifdef _WIN32
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    LOGE("WSAStartup failed");
    return false;
  }
#endif
  sock_t fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == BAD_SOCK) {
    LOGE("hotReload: socket failed (안드로이드: INTERNET 권한?)");
    return false;
  }
  int one = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof one);
  sockaddr_in ad{};
  ad.sin_family = AF_INET;
  ad.sin_port = htons((unsigned short)port);
  ad.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(fd, (sockaddr *)&ad, sizeof ad) < 0 || listen(fd, 2) < 0) {
    LOGE("hotReload: bind/listen failed (port %d 사용 중?)", port);
    closesock(fd);
    return false;
  }
  setNonBlocking(fd, true);
  a.listenFd = (std::intptr_t)fd;
  LOGI("hotReload: listening on 127.0.0.1:%d", port);
  return true;
}

void hotReloadPoll(App &a) {
  if (a.listenFd == -1)
    return;
  sock_t c = accept(toSock(a.listenFd), nullptr, nullptr);
  if (c == BAD_SOCK)
    return;
  setNonBlocking(c, false); // 윈도우는 accept 소켓이 non-blocking 을 상속함
#ifdef _WIN32
  DWORD tv = 2000;
#else
  timeval tv{2, 0};
#endif
  setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);

  std::string src;
  char buf[4096];
  bool done = false;
  while (!done) {
    int n = (int)recv(c, buf, sizeof buf, 0);
    if (n <= 0)
      break;
    for (int i = 0; i < n; i++) {
      if (buf[i] == '\0') {
        done = true;
        break;
      }
      src += buf[i];
    }
  }

  bool empty = src.find_first_not_of(" \t\r\n") == std::string::npos;
  std::string reply;
  if (empty) {
    reply =
        "error: " + (a.lastError.empty() ? std::string("none") : a.lastError) +
        "\n--- print ---\n" + a.printBuf;
    a.printBuf.clear();
  } else {
    reply = loadLuaScript(a, src);
    LOGI("hotReload: %s", reply.c_str());
  }
  reply += "\n";
  send(c, reply.data(), (int)reply.size(), 0);
  closesock(c);
}

void hotReloadClose(App &a) {
  if (a.listenFd != -1) {
    closesock(toSock(a.listenFd));
    a.listenFd = -1;
#ifdef _WIN32
    WSACleanup();
#endif
  }
}
