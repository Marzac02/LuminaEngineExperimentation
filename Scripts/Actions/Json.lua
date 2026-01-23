local json = {}

local function encode_string(str)
    str = str:gsub("\\", "\\\\")
    str = str:gsub('"', '\\"')
    str = str:gsub("\n", "\\n")
    str = str:gsub("\r", "\\r")
    str = str:gsub("\t", "\\t")
    return '"' .. str .. '"'
end

local function encode_table(tbl, indent)
    indent = indent or 0
    local indent_str = string.rep("  ", indent)
    local next_indent = string.rep("  ", indent + 1)
    
    local is_array = true
    local count = 0
    for k, v in pairs(tbl) do
        count = count + 1
        if type(k) ~= "number" or k ~= count then
            is_array = false
            break
        end
    end
    
    if is_array then
        local result = "[\n"
        for i, v in ipairs(tbl) do
            result = result .. next_indent .. json.encode(v, indent + 1)
            if i < #tbl then
                result = result .. ","
            end
            result = result .. "\n"
        end
        result = result .. indent_str .. "]"
        return result
    else
        local result = "{\n"
        local first = true
        for k, v in pairs(tbl) do
            if not first then
                result = result .. ",\n"
            end
            first = false
            result = result .. next_indent .. encode_string(k) .. ": " .. json.encode(v, indent + 1)
        end
        result = result .. "\n" .. indent_str .. "}"
        return result
    end
end

function json.encode(value, indent)
    local t = type(value)
    
    if t == "string" then
        return encode_string(value)
    elseif t == "number" then
        return tostring(value)
    elseif t == "boolean" then
        return value and "true" or "false"
    elseif t == "table" then
        return encode_table(value, indent)
    elseif t == "nil" then
        return "null"
    else
        error("Cannot encode type: " .. t)
    end
end

return json