

#Simple Modbus TCP server/client example:

modbus is a defined protocal which is widely used in communication between PC and PLC (programmable logic controller)

modbus TCP is one kind of modbus that use TCP network to translate, benefit from operating system's well establish network system, modbus TCP is more simple relative to other kind of modbus ( ex: RTU )

This is the simplest modbus TCP connection example , for those who may face to handcraft the modbus TCP protocol, this project keep as original TCP socket as possible to make copy-paste easily. 






#TCP modbus query example :

modbus mainly use uint16_16 as a unit, so the queries would looks in pair:

(In PLC world they tend to call server as slave, client as master)


when the connection establish , modbus TCP client would sending following request to server :

[0] [1] [0] [0] [0] [6] [1] [3] [0] [2] [0] [3]
|  A  ||   B   |   C   | D | E |   F   |   G   |


[A] Transaction Identifier (0,1) : 2 Bytes number to distinguish each query ,  usually an counting number whenever sending a query

[B] Protocol Identifier (0,0) : keep 0 here

[C] Length (0,6) : following data length

[D] Unit Identifier (1) : device ID of the modbus server (slave) , actually is a remind data from modbus RTU. Modbus TCP already has IP as identifer, but some people use this to do some fantasy function. How to response to untu identifer is totally based on the server side ( some of TCP modbus devices are just ignore them ) , keep an eyes on your server side to see if they care unit id or not.

[E] Function Code (3) : we use 3 means "Read the holding register", holding register is a simple place to put number inside.

[F] Starting Address (0,2) : read data from this

[G] Quantity of Registers (0,2) : how many registers this queries want to read, in this example is 03 , means we read address 02 , 03 , 04 ( totally 3 registers )





when server receive the request, it would response following query :

<0> <1> <0> <0> <0> <9> <1> <3> <6> <0> <6> <0> <7> <0> <8>
|  H  ||   I   |   J   | K | L | M |          N           |


[H] Transaction Identifier (0,1) : should be the same as [A]

[I] Protocol Identifier (0,0) : should be the same as [B]

[J] Length (0,9) : following data length

[K] Unit Identifier (1) : should be the same as [D]

[L] Function Code (3) : should be the same as [L]

[M] Byte Count (6) : following register length ( register quantity * 2 )

[N] Register data (0,6,0,7,0,8) : here we request 3 registers, is (0,6), (0,7) , and (0,8)








some (many!) modbus users used to treat modbus's queries' minimal unit as uint16, so when they define the "Big endian" and "Little endian" they may have different byte swap method inside each uint16 data compare to other system. keep an eyes on it or you may confused on what data you got when you translate the data from more than 1 registers.


###how to make

go to the folder client or server, with MAKEFILE side the folder

use command

```
./make
```


#p.s.

The main structure of the TCP select connection code is writed by ChatGPT them I modified it to modbus protocal format

the makefile is modified from the auto-generated makefile form qmake, the format is beautiful and I don't see any disadvantage inside. I keep the qmake pro file here if someone interested.

you can use libmodbus or modbus poll to verify the modbus's behavior




