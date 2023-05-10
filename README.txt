This program is an implemetation of a chat server that can handle up to 4 clients, but can be adjusted by allocating
more server threads and/or adjusting the NUM_CLIENTS constant in the server file to desired client limit. 

The server spawns a thread for each connected client.

If a client sends a message, its associated thread will read it and send it to both the client and all the other
connected clients, utilizing a global client fd array in the server side. 

Two threads are created on the client side, each with a separate thread function to write messages to the server
and read them concurrently. 

The server associates a username with each client, storing it in a global client array, and closing the client connection
from the server if it enters a username already in use. 

If the user enters \q, it will close the connection to the server and alert all other clients in the server that it has left.


In order to test this program:
------------------------------
1. Launch the server by entering the chatserver command 
2. open separate terminal windows for desired number of clients to test and enter the chatclient command in each.
3. Itll prompt for a username and if its not already in use, the client will be able to continue in the chat server.
4. If username is in use, you will have to reenter the chatclient command and input a new username. 

Limitations:
------------------------------
I found it very difficult initially to figure out a way to send a message to every client until I implemented the global client fd array
and then called the write function in a loop afterwards. 

I had some issues with utilizing the buffers and having input appear on the wrong line, the username not showing up, or being overwritten by another client.

Sometimes when I would enter in user input after succesfully connecting into the server, I would see random characters appear. I believe this has something to do 
with not properly clearing my buffers, or overrwriting old buffers.

Lastly, all of my warning, if not most, are due to declaration within the program, however, if I were to move them all to the beginning/ make them global I would
be left with tirelessly reconfiguring the entire program, trying to adjust scoping and whatnot, so I left them where they were at locally as is given that the overall
program functionality was still there. 

I could have added many more error checks both on the system side and on the user side, but again, this would have costed much more time. 

I would have liked to clear the user from the server's respective user arrays upon a client exiting but I ran out of time and didn't want to break the basic 
functionality. This server will simply store each subsequent client and then block all others when the client max has been reached. As clients exit, I don't believe
that they are being updated within the server to be unoccupied. This is a future improvement.