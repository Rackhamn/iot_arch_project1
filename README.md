# iot_arch_project1

For our 3-month long IoT Architecture course at Jensen YH (2025)  
We are 7 members.  
### Stockholm:
* Carl Blumenthal (Rackhamn)
* Mojtaba Mohseni (xxx)
* Swathi xxx (yyy)
* Gabriel xxx (yyy)
### Malmö:
* Martin af Uhr (stormtomten)
* Kerry xxx (yyy)
* Denize S. (Que-ctrl)

# Project Information
Designing a system assisting housekeeping and supervisors with post-checkout control for short-term apartment rentals.
  
## Plan
Initial Meeting: 2025-01-21, 17:00 _done_    
Meeting log: https://docs.google.com/document/d/1dKvJOzCxKPIAF0xGJFeyn81T_Q298DL61lQWgLDmnBw/edit?usp=sharing  

### Basic Architecture Idea:  
A main server with a database + user website.  
A Mesh network of pico connected active RFID tags.  
A set of passive RFID tags read by the mesh network.  
A Reader/Writer pair for RFID tags.  
  
## Details
Plans and details to follow
  
## Resources
[Link Example](http://google.com)  
[Github .md helper](https://gist.github.com/allysonsilva/85fff14a22bbdf55485be947566cc09e)  

```mermaid
graph TD
    Server[Server] --> |TCP/IPV4/6| Master
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
```

### Initial Whiteboard Sketch
![Whiteboard Sketch](https://github.com/Rackhamn/iot_arch_project1/blob/main/Resources/Screenshot%202025-01-23%20094007.png)
