# LiteBot
A highly efficient 24/7 Discord VC bot written in pure C.  
The bot is a statically linked, libc-free Linux x86-64 executable.

Features: (1) a ping command (ping the bot), and (2) plays an `.mp3` in a VC forever.

It uses minimal CPU and RAM, weighs approx < 0.1 MB at runtime and costs < £0.1/year in electricity (excludes network bandwidth).

> **\*🧠 AI DISCLAIMER:** AI was heavily used to write the C code. The code style and most comments made by the AI have deliberately been kept in, so that readers (or even other AIs) will be able to know what this AI was thinking, in case it made mistakes.\*

## File Structure
```bash
.
├─ assets/              # Audio files
│  └─ Pinnacle.bin      # Converted audio (generated)
│  ├─ Pinnacle.mp3      # Source audio
├─ build/               # Compiled binaries (generated)
│  ├─ bot               # Bot executable
│  ├─ bot.map           # Linker map
│  ├─ bot.o             # Bot object file
│  └─ convert           # Audio conversion tool
├─ src/                 # Source code
│  ├─ audio.h           # Shared audio settings
│  ├─ bot.c             # Entry point
│  ├─ convert.c         # MP3 -> .bin tool
│  ├─ dave.h            # DAVE (MLS) protocol
│  ├─ dns.h             # DNS resolver
│  ├─ json.h            # JSON parser
│  ├─ mcrypto.h         # Mini Crypto primitives
│  ├─ mp3.h             # MP3 decoder
│  ├─ mtls.h            # Mini TLS 1.3 client
│  ├─ opusenc.h         # Opus encoder
│  ├─ p256.h            # P-256 elliptic curve
│  ├─ stdlite.h         # Stdlib replacements
│  └─ syslite.h         # Syscall wrappers
├─ .gitignore           # Hides certain files
├─ config-example.json  # Example configuration
├─ Makefile             # Build script
├─ monitor.sh           # Resource tracker
└─ README.md            # Defines this README page
```
Note: Most of the files like `dave.h` are partial implementations.

## How to use:
1. Clone this repository: `git clone https://github.com/imptype/LiteBot`

2. Rename `config-example.json` to `config.json` and update it:
   - `token` is a Discord bot token, get one via [Discord's Developer Portal][1].
   - `guild_id` is a Discord server ID, make sure the bot is in this server too.
   - `vc_channel_id` is a Discord voice channel ID, located in the above guild.
   - `owner_id` is an optional Discord user ID, to make the ping command owner-only.
   - `mp3_file` is the name of the `.mp3` file without the extension.

3. Setup everything with `make` then do `build/bot` to run.

Done! The bot is now in a VC, and you can ping it for a reply.  
You can also run `sh monitor.sh` to check resource stats.

## Credits / Attribution
If you are going to use this bot as unchanged, make sure you attribute [Pinnacle by Alegend][2] (see [FreeToUse's Usage Policy][3]). 

To properly attribute, you could either:
- Put a note in the bot's bio.
- Set the voice channel status text.
- Add text in the guide channel of the server.

*This repository is licensed under the [MIT License][4].*

<!-- References -->
[1]: https://discord.com/developers/applications
[2]: https://freetouse.com/music/alegend/pinnacle
[3]: https://freetouse.com/usage-policy
[4]: LICENSE
