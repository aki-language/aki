-- Copyright (C) 2024 Vincent Hengel.
-- For licensing information see LICENSE at the root of this distribution.

local p = premake
local api = p.api

p.modules.aki = {}
p.modules.aki._VERSION = p._VERSION

local aki = p.modules.aki

-- where to put the generated files
api.register {
  name = "akigendir",
  scope = "config",
  kind = "directory",
}

-- generates a json that is parsed to the aki tool
function aki.generateToolJSON(file_list)
    local json_entries = {}
    for i, file in ipairs(file_list) do
        local entry = string.format('{"path":"%s","commands":[]}', file)
        table.insert(json_entries, entry)
    end

    return string.format("[\n  %s\n]", table.concat(json_entries, ",\n  "))
end

function aki.collectAkiFiles(prj)
  for cfg in premake.project.eachconfig(prj) do
    local files = {}
    for _, file in ipairs(cfg.files) do
      if file:endswith(".aki") then
        table.insert(files, file)
      end
    end
    cfg.aki_files = files
  end
end

premake.override(premake.project, "bake", function(oldfn, prj)
    -- bake the regular content first
    oldfn(prj)

    aki.collectAkiFiles(prj)

    for cfg in premake.project.eachconfig(prj) do
      if #cfg.aki_files > 0 then
        local json = aki.generateToolJSON(cfg.aki_files)
        print(cfg.akigendir)
        local path = premake.project.getrelative(cfg.project, cfg.akigendir)
        -- append the json name:
        path = path .. "/aki.json"
        print(path)
        local file = io.open(path, "w")
        if file then
            file:write(json)
            file:close()
        else
            error("Could not open file for writing: " .. path)
        end
      end
    end

    -- todo: run the transpiler
    -- TODO: insert the generaterated files into the file lists.
end)
