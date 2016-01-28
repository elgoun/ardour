ardour {
	["type"]    = "session",
	name        = "Example",
	license     = "MIT",
	author      = "Robin Gareus",
	email       = "robin@gareus.org",
	site        = "http://gareus.org",
	description = [[
	An Example Ardour Session Process Plugin.
	Install a 'hook' that is called on every process cycle
	(before doing any processing).]]
}

function sess_params ()
	return
	{
		["print"]  = { title = "Print (yes/no)", default = "yes", optional = false },
		["name"] = { title = "Print Prefix", default = "", optional = true },
	}
end

function factory (params)
	return function ()
		p = params["print"] or "no"
		n = params["name"] or "Test"
		if p ~= "yes" then return end
		a = a or 0
		a = a + 1
		print (n, a, Session:transport_rolling())
	end
end
