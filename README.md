# Implementation-of-GBN-and-SR-Protocols-in-Computer-Networks

SENDER:
Our objective at sender program is that it should take the following command line arguments: input file, port number on which receiver is connected and number of packets you want to send. Using the parameters passed, the sender will communicate with the receiver process using UDP. During the session, the sender will display the following information for each segment sent and received:
1. The sequence number of the segment sent to the receiver and timer information.
2. Display the acknowledgement number received from the receiver.
3. When the timer for a particular sequence number expires, it should resend segments according to the protocol you are implementing. Display relevant information appropriately.

RECEIVER:
Our objective at receiver program is that it should take the following command line arguments: input file (so you can determine the type of protocol to use) and the port number the receiver will run on.
1. Create a socket with the specified port number and it will wait for client.
2. Display the sequence numbers that are received.
3. Send the acknowledgement according to protocol

IMPLEMENTATION:
The application will be a command line interface (CLI), and we will need to pass an input file, which will contain the following inputs:
1. Protocol Name: GBN or SR
2. m N; where m = no of bits used in sequence numbers, N = Window size
3. Timeout period in micro second/millisecond. Use your judgement to set the value during simulation (4 seconds is
good for testing).
4. Size of the segment in bytes

EXAMPLE 1:
GBN
4 15
10000000
500

EXAMPLE 2:
SR 
4 8 
10000000 
500
