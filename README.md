# JATS - Jensen Asset Tracking Solution

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


### Update:  
_We are going to have to change our original architecture idea slightly._ :thinking:
_Instead of having an active RF reader in every room that communicates with the RFID tags, we are going to have a RFID reader module that a person can use to scan all the tags in a room._
_Each read will be sent through our mesh network that in turn speaks to the server._

_It might be best to have a split architecture._  
_One with the basic passive RFID tags where we use a reader module._  
_One with the active RFID tags that we try to read once every N seconds. Need to find a working antenna though + think about power consumtion._  
  
## Details
Plans and details to follow :smiley:
  
## Resources
[Link Example](http://google.com)  
[Github .md helper](https://gist.github.com/allysonsilva/85fff14a22bbdf55485be947566cc09e)  
  
<br/><br/><br/><br/>
  
### Architecture Diagrams:  

```mermaid
graph TD
    ServerCache@{ shape: docs, label: "Cache<br/><p style="color: orange;"><b><i>Redis</i></b></p>" } <--> Server
    Server@{ shape: docs, label: "Server Backend<br/><p style="color: orange;"><b><i>Parallel Job Pool</i></b></p>" } <--> |TCP/IPV4/6| Master
    Server <--> |TCP/IPV4/6| Master2
    Server <--> DB@{ shape: cyl, label: "Database<br/><p style="color: orange;"><b><i>SQLite3</i></b></p>" }
    Server --> Anal@{ shape: processes, label: "Analytics<br/><p style="color: orange;"><b><i>Grafana?</i></b></p>" }
    DB <--> BD2@{ shape: cyl, label: "Parity Copy<br/>Database" }

    Anal --> Website[Website]
    %% Website <--> User@{ shape: hex, label: "User<br/><p style="color: orange;"><b><i>APT Owner</i></b></p>"}

    Server <--> Logger@{ shape: docs, label: "Log Service<br/><p style="color: orange;"><b><i>+ Watchdog<br/>+ Wireshark</i></b></p>" }
    Server <--> ServerHTML@{ shape: docs, label: "HTML Server<br/><p style="color: orange;"><b><i>Apache / nginx</i></b></p>" }
    ServerHTML <--> Website
    
    Master <--> A1(Mesh AP)
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
    subgraph APT_1
        Master(Master<br/>Mesh AP)
        A1
        A2
        A3
        P1
        P2
    end

  Master2 <--> Mesh2_1
  Master2 <--> Mesh2_2
  Master2 <--> Mesh2_3
  Mesh2_1 <--> Mesh2_2
  Mesh2_2 <--> Mesh2_3
  Mesh2_3 <--> Mesh2_1
  PT1 <--> Reader
  PT2 <--> Reader
  PT3 <--> Reader
  PT4 <--> Reader

  %% Housekeeping@{ shape: hex, label: "Housekeeping<br/><p style="color: orange;"><b><i>Security</i></b></p>"} --> Reader
  Reader(RFID<br/>Interface<br/>Module<br/><p style="color: orange;"><b><i>MFRC522</i></b></p>) --> Mesh2_1
  subgraph APT_2
      Master2(Master<br/>Mesh AP)
      Mesh2_1(Nearest<br/>Mesh AP)
      Mesh2_2(Mesh AP)
      Mesh2_3(Mesh AP)
      PT1((Passive Tag))
      PT2((Passive Tag))
      PT3((Passive Tag))
      PT4((Active Tag))
    end
```

<br/><br/>
---  

### Basic Handheld Reader - Mesh - Server (RMS) Sequence
```mermaid
sequenceDiagram
    Note left of Reader: Read Tag Info Successfully
    Note left of Mesh: Move Packet Payload<br/>Through Mesh Network<br/>To Master Mesh
    Reader ->> Mesh: [ Tag ID + Info ]
    Mesh -->> Server: [ Tag ID + Info ] + [ Mesh Info ]
    Note right of Server: Fetch DB info from TAG + Mesh info<br/>Store & Update Tag LRU info
    Server --x Mesh: [ Tag DB Info ] + [ Reader ID ]
    Mesh -x Reader: [ Tag DB Info ]
    Note left of Reader: Now Display Info on Reader Screen<br/>Store Info Locally In List
```

<br/><br/>
### First Approximation of RMS Packet Payload
```basic
0      64     128    144    152    168    192    225    255
[ hid  | tps  | tnb  [ blid | size | data | crc  ] zpad ]
```

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
    64-127: "Total Payload Size : 64b" 
    128-143: "Total Num Blocks : 16b"
    144-151: "Block ID : 8b"
    152-167: "Block Length : 16b"
    168-191: "Data (Var Len) : (N - 32)b"
    192-224: "Block Checksum : 32b"
    225-255: "Zero Fill Padding To Next Pow2 - Discard/Dropped"
```


<br/><br/>
---  

### RFID Reader States
&nbsp;&nbsp;&nbsp;&nbsp;_plz reduce complexity here_  

```mermaid
stateDiagram-v2
    [*] --> OFF
    OFF --> Booting : Power On
    Booting --> Idle : Boot Complete
    Idle --> Connecting : Login, Auth, Connect<br/>to Mesh/Server
    Connecting --> Connected : Connection Successful
    Connecting --> Idle : Connection Failed
    Connected --> Scanning : Scan for Tags
    Scanning --> Verifying : Tag Scanned
    Verifying --> FetchingDBInfo : Verification OK
    Verifying --> Error : Verification Failed
    FetchingDBInfo --> StoringDBInfo : DB Info Fetched
    FetchingDBInfo --> Error : DB Fetch Failed
    StoringDBInfo --> Idle : Info Stored
    Idle --> LoadingDBInfo : Load Stored DB Info
    LoadingDBInfo --> DisplayingDBInfo : Info Loaded
    DisplayingDBInfo --> Idle : Display Complete
    Connected --> Error : Connection Lost
    Error --> Idle : Reset to Idle
    Idle --> OFF : Power Off
    [*] --> Crash : Unexpected Error
    Crash --> OFF : Restart

    state Connected {
        [*] --> Ready
        Ready --> Syncing : Sync with Server
        Syncing --> Ready : Sync Complete
        Syncing --> Error : Sync Failed
        Ready --> Sleeping : Low Power Mode
        Sleeping --> Ready : Wake Up
    }
```

<br/><br/>

---  

### Initial Whiteboard Sketch
![Whiteboard Sketch](https://github.com/Rackhamn/iot_arch_project1/blob/main/Resources/Screenshot%202025-01-23%20094007.png)
