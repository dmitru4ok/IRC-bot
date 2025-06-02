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
16. 

## Challenges
1. C strings
2. Main problem - message parsing. Especially if messages break in the middle because of buffer size.
3. How to build architecture? 
4. Children inherited the same socket and main was reading their input instead of listening to the network (fixed by only allowing main to network)
5. 
