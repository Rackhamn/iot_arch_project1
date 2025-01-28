# iot_arch_project1

For our 3-month long IoT Architecture course at Jensen YH (2025)  
We are 7 members.  
### Stockholm:
* Carl Blumenthal (Rackhamn)
* Mojtaba Mohseni (Mohsen-png)
* Swathi xxx (swati72-t2)
* Gabriel xxx (Gabbemannen00)
### Malmö:
* Martin af Uhr (stormtomten)
* Kerry xxx (yyy)
* Denize S. (Que-ctrl)

# Project Information
_Designing a system assisting housekeeping and supervisors with post-checkout control for short-term apartment rentals._
  
## Plan
Initial Meeting: 2025-01-21, 17:00 _done_    
Meeting log: https://docs.google.com/document/d/1dKvJOzCxKPIAF0xGJFeyn81T_Q298DL61lQWgLDmnBw/edit?usp=sharing  

### Basic Architecture Idea:  
A main server with a database + user website.  
A Mesh network of pico connected active RFID tags. 
It should configure itself and choose one as Master (that interacts with the server and runs commands)   
A set of passive RFID tags read by the mesh network.  
A Reader/Writer pair for RFID tags.  


Update:  
We are going to have to change our original architecture idea slightly.  
Instead of having an active RF reader in every room that communicates with the RFID tags, we are going to have a RFID reader module that a person can use to scan all the tags in a room.  
Each read will be sent through our mesh network that in turn speaks to the server.

It might be best to have a split architecture.  
One with the basic passive RFID tags where we use a reader module.  
One with the active RFID tags that we try to read once every N seconds. Need to find a working antenna though + think about power consumtion.  
  
## Details
Plans and details to follow
  
## Resources
[Link Example](http://google.com)  
[Github .md helper](https://gist.github.com/allysonsilva/85fff14a22bbdf55485be947566cc09e)  

### TODO:  
  Server-Master Sequence Diagram  
  Mesh Sequence Diagram  


```mermaid
graph TD
    Server[Server] --> |TCP/IPV4/6| Master
    Server --> |TCP/IPV4/6| Master2
    Server --> DB[Database]
    Server --> Anal[Analytics]
    DB --> Server
    DB --> Anal
    Anal --> Website[Website]
    Server --> Website
    Website --> Server
    Master --> A1((Active Tag))
    Master --> A2((Active Tag))
    Master --> A3((Active Tag))
    Master --> P1((Passive Tag))
    Master --> P2((Passive Tag))
    A1 --> P1
    A1 --> P2
    A2 --> P1
    A2 --> P2
    A3 --> P1
    A3 --> P2
    subgraph lägenheten
        Master
        A1
        A2
        A3
        P1
        P2
  end

  Master2 --> Mesh2_1
  Master2 --> Mesh2_2
  Master2 --> Mesh2_3
  Mesh2_1 --> Mesh2_2
  Mesh2_2 --> Mesh2_3
  Mesh2_3 --> Mesh2_1
  PT1 --> Reader
  PT2 --> Reader
  PT3 --> Reader
  PT4 --> Reader
  Reader --> Mesh2_1
  subgraph lägenheten2
      Master2
      Mesh2_1
      Mesh2_2
      Mesh2_3
      PT1
      PT2
      PT3
      PT4
    end
```

### Basic Handheld Reader - Mesh - Server (RMS) Sequence
```mermaid
sequenceDiagram
    Note left of Reader: Read Tag Info Successfully
    Note left of Mesh: Move Packet Payload To Master Mesh
    Reader ->> Mesh: [ Tag ID + Info ]
    Mesh -->> Server: [ Tag ID + Info ] + [ Mesh Info ]
    Note right of Server: Fetch DB info from TAG + Mesh info
    Server --x Mesh: [ Tag DB Info ] + [ Reader ID ]
    Mesh -x Reader: [ Tag DB Info ]
    Note left of Reader: Now Display Info on Reader Screen
```

### First Approximation of RMS Packet Payload
```mermaid
---
title: "Example RMS Packet Payload"
config:
    packet:
        rowHeight: 32
        bitWidth: 16
        bitsPerRow: 64
        showBits: true
        paddingX: 10
        paddingY: 5
---
 packet-beta
    0-63: "Hashed ID : 64b"
    64-127: "Payload Size : 64b" 
    128-143: "Num Blocks : 16b"
    144-151: "Block ID : 8b"
    152-167: "Block Length : 16b"
    168-191: "Data (Var Len) : Nb"
```
### Initial Whiteboard Sketch
![Whiteboard Sketch](https://github.com/Rackhamn/iot_arch_project1/blob/main/Resources/Screenshot%202025-01-23%20094007.png)
