-- 저장 후 ./push.sh 하면 앱에 바로 반영됨
-- API: clear_color(r,g,b)  rect(x,y,w,h, r,g,b)  print(...)
-- 좌표: 화면 중앙 (0,0), 가로/세로 -1 ~ 1

local t = 0

function update(dt)
  t = t + dt
  clear_color(0.0, 0.0, 0.0)

  setFloat("uTime", t)
  setFloat("uToffset", 0.06)
  setFloat("uboost", 2.0)
  setFloat("ucontrast", 0.8)
  setFloat("uConCenter", 0.8)
  setFloat("uradius", 0.5)
  setFloat("ufadeRad", 0.5)
  setFloat("NOISE_AMT", 0.6)
  setTrans( t*t, vec3.new(0.0, 0.0, 1.0),
                   vec3.new(0.5, -0.5, 0.0));


  local a = t * 2 * math.pi / 5   -- 5초에 한 바퀴
  rect(vec4.new(0, 0, 6, 13),
       vec3.new(0.5 + math.sin(a), 0.6, 0.85))
end
