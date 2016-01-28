ardour {
	["type"]    = "dsp",
	name        = "Simple Amp III",
	license     = "MIT",
	author      = "Robin Gareus",
	email       = "robin@gareus.org",
	site        = "http://gareus.org",
	description = [[
	An Example DSP Plugin for processing audio, to
	be used with Ardour's Lua scripting facility.]]
}

function dsp_ioconfig ()
	return
	{
		{ audio_in = -1, audio_out = -1},
	}
end


function dsp_params ()
	return
	{
		{ ["type"] = "input", name = "Gain", min = -20, max = 20, default = 6, unit="dB", scalepoints = { ["0"] = 0, ["twice as loud"] = 6 , ["half as loud"] = -6 } },
	}
end


-- use ardour's vectorized functions
--
-- This is as efficient as Ardour doing it itself in C++
-- Lua function call overhead is tiny

function dsp (bufs, in_map, out_map, n_samples, offset)
	local n_channels = bufs:count():n_audio()
	local ctrl = CtrlPorts:array() -- get control port array
	local gain = ARDOUR.DSP.dB_to_coefficient (ctrl[1])
	for c = 1,n_channels do
		-- TODO we should look up the channel map
		ARDOUR.DSP.apply_gain_to_buffer (bufs:get_audio(c - 1):data(0), n_samples, gain); -- process in-place
	end
end
