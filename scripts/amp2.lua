ardour {
	["type"]    = "dsp",
	name        = "Simple Amp II",
	license     = "MIT",
	author      = "Robin Gareus",
	email       = "robin@gareus.org",
	site        = "http://gareus.org",
	description = [[
	An Example DSP Plugin for processing audio, to
	be used with Ardour's Lua scripting facility.]]
}

function dsp_ioconfig ()
	return { [1] = { audio_in = -1, audio_out = -1}, }
end

-- this variant modifies the audio data in-place
-- in Ardour's buffer.
--
-- every assignment (in-place) calls a c-function behind
-- the scenes to get/set the value.
-- Still, it's a bit more efficient than "Amp I" on most systems.

function dsp (bufs, in_map, out_map, n_samples, offset)
	local n_channels = bufs:count():n_audio();
	for c = 1,n_channels do
		local a = bufs:get_audio(c - 1):data(0):array() -- get a reference (pointer to array)
		for s = 1,n_samples do
			a[s] = a[s] * 2; -- modify data in-place (shared with ardour)
		end
	end
end
