#ifdef __ANDROID__
lua.set_function("print", [](sol::variadic_args va, sol::this_state s) {
  sol::state_view L(s);
  std::string out;
  for (auto v : va) {
    out += L["tostring"](v).get<std::string>();
    out += '\t';
  }
  __android_log_print(ANDROID_LOG_INFO, "lua", "%s", out.c_str());
});
#endif
