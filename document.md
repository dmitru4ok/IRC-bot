# Requirements
1. libcurl4-openssl-dev
2. libjansson-dev
3. ollama + gemma3:1b @ localhost:5000 (values are hard-coded, not configurable :P)
4. Linux, I guess

## Usage
1. Bot will join the channels listed in the bot.cfg file (comma-separated). Channels, protected by passwords are not supported.
2. Narrative keywords are used in bot's AI prompts to give better answers.
3. Narratives are specified in the config file, comma-separated. Length of narratives list and channel list should be identical.
4. In the list of narratives first channel gets first narrative, second channel - second. etc. E.g
    channels=#Unix,#news
    narratives=Operating systems,World politics

    channel #Unix gets the "Operating systems" narrative, #news gets "World politics"
5. Admin channel - password-protected channel that allows switching narratives for the channel and gracefully shutting dowon the bot.
    For changing narrative: privmsg <#admin_channel> :switch <channel_to_switch> <new narrative> (mind, only singe space is supported between args)
    To shutdown the bot gracefully (Bot will send QUIT msg to the server): privmsg <#admin_channel> :quit (Generally, string should start with 'quit')
6. **Limitations**: bot doesn't really check if channel joinin is successful and does not support password-protected channels (except for admin channel). Bot doesn't track joins and parts, so it really never knows if it is alone in the channel. Bot can talk to channels only, no direct user messages. No notices, either.

## Steps
1. Read RFC 1459, get into the server using nc, play with most commands manually.
2. Learn fork, pipe and semaphores basics. Implement simple print-programs.
3. Parse confic
3. Connect to the server using socket, get MOTD printed onto screen.
4. Controlled fork in a loop, establish 1 set of pipes
5. Refactor, build proof of concept architecture, add second set of pipes for bidirectional communication
6. Ping-Pong, on bot's own.
7. message parsing (finally).
8. parse out sender's nick from prefix
9. Shared-mem semaphore for logging and loggin itself + config updates.
10. Ignore bots when replying to PRIVMSGs
11. Add admin channel creation
12. Add quit command (on admin channel) for graceful termination + free resources on graceful term
13. Move config variable to shared memory, add narratives array to config, narrative parsing.
14. Narrative change via admin command
15. Make main respond to privmsg, not only log the answer
16. integrate with AI: add curl and jannson libs, link in Makefile too

## Challenges
1. C strings
2. Main problem - message parsing. Especially if messages break in the middle because of buffer size.
3. How to build architecture?
4. Do I just ignore MOTD without parsing? (I did)
5. Children inherited the same socket and main was reading their input instead of listening to the network (fixed by only allowing main to network)
6. Handling JSON from API is sheer pain.
7. I don't check for all the errors: is nick already taken? Did I join channel successfully?
8. Also, temptation to hardcode buffre sizes, not define them
9. Apparently, there is a bug were some command that should not be sent gets formed of leftovers of AI call.

