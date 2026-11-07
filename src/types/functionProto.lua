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

---@param name string
---@param v boolean
function setBool(name, v) end

---@param name string
---@param v integer
function setInt(name, v) end

---@param name string
---@param v number
function setFloat(name, v) end

---@param name string
---@param v vec3
function setVec3(name, v) end

---@param name string
---@param v vec4
function setVec4(name, v) end

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

---@param rect vec4
---@param color vec3
function rect(rect, color) end
