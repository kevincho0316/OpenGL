---@meta

---@class vec3
---@field x number
---@field y number
---@field z number
vec3 = {}

---@param x number
---@param y number
---@param z number
---@return vec3
function vec3.new(x, y, z) end

---@class vec4
---@field x number
---@field y number
---@field z number
---@field w number
vec4 = {}

---@param x number
---@param y number
---@param z number
---@param w number
---@return vec4
function vec4.new(x, y, z, w) end

---@param r number
---@param g number
---@param b number
function clear_color(r, g, b) end

---@param shader string
---@param name string
---@param v boolean
function setBool(shader, name, v) end

---@param shader string
---@param name string
---@param v integer
function setInt(shader, name, v) end

---@param shader string
---@param name string
---@param v number
function setFloat(shader, name, v) end

---@param shader string
---@param name string
---@param v vec3
function setVec3(shader, name, v) end

---@param shader string
---@param name string
---@param v vec4
function setVec4(shader, name, v) end

---@param angle number
---@param axis vec3
---@param scale vec3
function setTrans(angle, axis, scale) end

---@param p vec3
function camPos(p) end

---@param yaw number
---@param pitch number
function camRot(yaw, pitch) end

---@param target vec3
function camLookAt(target) end

---@param d vec3
function camMove(d) end

---@param fov number  기본 45
function camPerspective(fov) end

---@param size number  기본 1
function camOrtho(size) end

---@param near number
---@param far number
function camClip(near, far) end

---@return vec3
function camGetPos() end

---@return vec3
function camFront() end

---@param shader string
---@param rect vec4
---@param color vec3
function rect(shader, rect, color) end

---@param path string
---@param size number
function setFont(path, size) end

---@param text string
---@param x number
---@param y number
---@param size number
---@param color vec4
function setText(text, x, y, size, color) end

---@param n integer  specter particle count (default 40)
function specterCount(n) end

---@param min number  seconds (default 1)
---@param max number  seconds (default 2)
function specterLifetime(min, max) end

---@param s number  quad UV runs -s..+s (default 1, UV_SCALE)
function specterUvScale(s) end

---@param mode integer  0 = final, 1 = one specter() channel grey, 2 = polar
function specterDebug(mode) end

---@param on boolean  HDR target + bloom/tonemap on/off
function postEnable(on) end

---@param threshold number  default 1.0
---@param strength number  default 1.0
---@param size number  0..1, default 0.5
function postBloom(threshold, strength, size) end

---@param s number  saturation boost, default 1.3
function postSaturation(s) end

---@param mode integer  0 = clamp only, 1 = ACES
function postTonemap(mode) end
