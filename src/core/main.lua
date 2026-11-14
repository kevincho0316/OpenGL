local t = 0
function init()
    -- setFont('fonts/pixel.ttf', 35.0)
end

function update(dt)
    t = t + dt
    clear_color(0.0, 0.0, 0.0)

    setFloat("main", "uToffset", 0.045)
    setFloat("main", "uboost", 3.0)
    setFloat("main", "ucontrast", 1.0)
    setFloat("main", "uConCenter", 0.9)
    setFloat("main", "uradius", 0.5)
    setFloat("main", "ufadeRad", 0.5)
    setFloat("main", "NOISE_AMT", 0.6)

    setTrans(180, vec3.new(0.0, 0.0, 1.0), vec3.new(0.5, -0.5, 0.0));



    -- 카메라 테스트: 원점 주위를 공전하면서 위아래로 흔들림
    camPerspective(60)
    local orbit = t * 0.1
    local radius = 7
    local cp = vec3.new(0.0, 0.0, 1.0) --vec3.new(math.sin(orbit) * radius * math.sin(t*0.2), math.sin(t * 0.5) * 3, math.cos(orbit) * radius)

    camPos(cp)
    camLookAt(vec3.new(0, 0, 0))

    setText("x: " .. string.format("%.2f", cp.x), 250.0, 250.0, 5, vec4.new(1, 1, 1, 1))
    setText("y: " .. string.format("%.2f", cp.y), 250.0, 400.0, 5, vec4.new(1, 1, 1, 1))
    setText("z: " .. string.format("%.2f", cp.z), 250.0, 550.0, 5, vec4.new(1, 1, 1, 1))

    local a = t * 2 * math.pi / 5 -- 5초에 한 바퀴
    rect("main", vec4.new(0, 0, 9, 13),
        vec3.new(0.5 + math.sin(a), 0.6, 0.85))
end
