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
[ARMv6-M PDF](https://developer.arm.com/documentation/ddi0419/latest/)  
  
<br/><br/><br/><br/>
  
## Architecture Diagrams:  
### Architecture For Server Diagrams:  
```mermaid
graph LR
    ServerCache@{ shape: docs, label: "Cache<br/><p style="color: orange;"><b><i>Redis</i></b></p>" } <--> Server
    Server@{ shape: docs, label: "Server Backend<br/><p style="color: orange;"><b><i>Parallel Job Pool</i></b></p>" }
    Server <--> DB@{ shape: cyl, label: "Database<br/><p style="color: orange;"><b><i>SQLite3</i></b></p>" }
    Server --> Anal@{ shape: processes, label: "Analytics<br/><p style="color: orange;"><b><i>Grafana?</i></b></p>" }
    DB <--> BD2@{ shape: cyl, label: "Parity Copy<br/>Database" }

    Anal --> Website[Website]
    %% Website <--> User@{ shape: hex, label: "User<br/><p style="color: orange;"><b><i>APT Owner</i></b></p>"}

    Server <--> Logger@{ shape: docs, label: "Log Service<br/><p style="color: orange;"><b><i>+ Watchdog<br/>+ Wireshark</i></b></p>" }
    Server <--> ServerHTML@{ shape: docs, label: "HTML Server<br/><p style="color: orange;"><b><i>Apache / nginx</i></b></p>" }
    ServerHTML <--> Website@{ shape: docs, label: "Website<br/><b style="color: orange;"><i>htmx</i></b></p>" }
    Apartment(Apartment<br/>or Mesh AP) <--> Server
```
<br/><br/>
---  

### Architecture For Apartments Diagrams:  
```mermaid
graph TD
    Server@{ shape: processes, label: "<b style="color: orange;"><i>Server</i></b>" }
    Server <--> |TCP/IPV4/6| Master
    Server <--> |TCP/IPV4/6| Master2

    Master <--> MESH_AP1(Nearest UHF Mesh AP)
    Master <--> MESH_AP2(UHF Mesh AP)
    Master <--> MESH_AP3(UHF Mesh AP)
    MESH_AP1 <--> MESH_AP2
    MESH_AP1 <--> MESH_AP3
    MESH_AP3 <--> MESH_AP2
    A1((Active Tag)) --> MESH_AP1
    A2((Active Tag)) --> MESH_AP1
    A3((Active Tag)) --> MESH_AP1
    A4((Active Tag)) --> MESH_AP1
    subgraph Apartment Active RFID
        Master(Master<br/>UHF Mesh AP)
        A1
        A2
        A3
        A4
        MESH_AP1
        MESH_AP2
        MESH_AP3
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
  subgraph Apartment Passive RFID
      Master2(Master<br/>Mesh AP)
      Mesh2_1(Nearest<br/>Mesh AP)
      Mesh2_2(Mesh AP)
      Mesh2_3(Mesh AP)
      PT1((Passive Tag))
      PT2((Passive Tag))
      PT3((Passive Tag))
      PT4((Passive Tag))
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
---  

<br/><br/>
### First Approximation of RMS Packet Payload
```c
0      64     128    144    152    168    192    225    255
[ hid  | tps  | tnb  [ blid | size | data | crc  ] zpad ]

/*
  64b := the max size of an integer is 64 bits (or 8 bytes)
  this means the following ranges:
    signed min:    -9,223,372,036,854,775,808         (-2^63)
    signed max:     9,223,372,036,854,775,807         (2^63 - 1)
    unsigned min:   0
    unsigned max:   18,446,744,073,709,551,615        (2^64 - 1)
*/

struct rms_block_64b_s {
  uint8_t     id;
  uint16_t    size;
  uint8_t *   data;
  uint32_t    crc32;
};
typedef struct rms_block_64b_s block_t;

struct rms_payload_64b_s {
  uint64_t    hash_id;
  uint64_t    size; // payload_size_bytes
  uint16_t    num_blocks;
  block_t *   blocks;
  uint8_t *   zero_padding;
};
typedef struct rms_payload_64b_s payload_t;
```

```mermaid
---
title: "Example 64b RMS Packet Payload"
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

---  

```c
0      32     64     72     80     96     128    160    192
[ hid  | tps  | tnb  [ blid | size | data | crc  ] zpad ]

/*
  32b := the max size of an integer is 32 bits (or 4 bytes)
  this means the following ranges:
    signed min:    -2,147,483,648                     (-2^31)
    signed max:     2,147,483,647                     (2^31 - 1)
    unsigned min:   0
    unsigned max:   4,294,967,295                     (2^32 - 1)
*/

struct rms_block_32b_s {
  uint8_t     id;
  uint16_t    size;
  uint8_t *   data;
  uint32_t    crc32;
};
typedef struct rms_block_32b_s block_t;

struct rms_payload_32b_s {
  uint32_t    hash_id;
  uint32_t    size; // payload_size_bytes
  uint8_t     num_blocks;
  block_t *   blocks;
  uint8_t *   zero_padding;
};
typedef struct rms_payload_32b_s payload_t;
```

```mermaid
---
title: "Example 32b RMS Packet Payload"
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
    0-31: "Hashed ID : 32b"
    32-63: "Total Payload Size : 32b" 
    64-71: "Total Num Blocks : 8b"
    72-79: "Block ID : 8b"
    80-95: "Block Length : 16b"
    96-127: "Data (Var Len) : (N - 32)b"
    128-159: "Block Checksum : 32b"
    160-191: "Zero Fill Padding To Next Pow2 - Discard/Dropped"
```

<br/><br/>

## RFID Reader Communication Layout
```mermaid
---
title: RFID Reader Communications Layout
---
graph
    PICO("<b style="color: orange;"><i>32-bit ARM Cortex-M0+<br/>Basic ARMv6-M instr.</i></b><br/>Pico W") <-- I2C<br/>SDA/SCL pins --> RFID_MODULE("<b style="color: orange;"><i>Address: 0xXX<br/>MFRC522 13.56 MHz</i></b><br/>RFID MODULE")
    PICO <-- I2C<br/>SDA/SCL pins --> LCD_MODULE("<b style="color: orange;"><i>Address: 0x27<br/>1602IIC / TC1602B-01</i></b><br/>LCD MODULE")
    RFID_MODULE <--> RFID_PTAG(Passive RFID Tag)
    PICO <--> SHA256_CODEC("<b style="color: orange;"><i>SHA256 / CRC32 Codec</i></b><br/>Packet Security")
    SHA256_CODEC <-- Builtin<br/>WiFi 802.11n --> MESH_AP(Nearest Mesh AP)
    MESH_AP <-- Mesh Network --> ROUTER(Apartment Router)
    ROUTER <-- TCP/IP --> Server(Server)
    PICO <--> MEMORY_CODEC("<b style="color: orange;"><i>LZ77 / RLE / DEFLATE</i></b><br/>Memory Codec")
    MEMORY_CODEC <-- SPI --> EEPROM("<b style="color: orange;"><i>External EEPROM / SSD</i></b><br/>or<br/><b style="color: orange;"><i>1MB Interal Flash Memory</i></b><br/>")
    MEMORY_CODEC <-- SPI --> RAM("<b style="color: orange;"><i>External SRAM</i></b><br/>or<br/><b style="color: orange;"><i>192KB Internal SRAM</i></b><br/>")
    PICO <--> VTABLE("<b style="color: orange;"><i>4KB Mapped Pages<br/>or Ring Buffer<br/>has Direct Memory Access</i></b><br/>Virtual Memory")
    VTABLE <--> MEMORY_CODEC
  %% <p style="color: orange;"><b><i>Redis</i></b></p>
```

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

## Configuration Sequence (Master Node & Phone/Laptop)
```mermaid
sequenceDiagram
    participant Phone/Laptop
    participant Master_Node as Master Node (Pico W)

    %% Master_Node ->> Master_Node: Init as AP Mode
    %% Mesh_Node ->> Mesh_Node: Init as STA Mode

    Phone/Laptop ->> Master_Node: Connect to AP (SSID: PicoMeshMaster)
    Phone/Laptop ->> Master_Node: Open Web UI for Configuration
    Phone/Laptop ->> Master_Node: Enter Mesh SSID & Password
    Master_Node ->> Phone/Laptop: Save & Confirm
    Master_Node ->> Master_Node: Switch to STA Mode
    Master_Node ->> Mesh_Node: Connect to Mesh Node (SSID: MeshNode)
```

## Mesh Communication Sequence (Master Node & Mesh Node)
```mermaid
sequenceDiagram
    participant Master_Node as Master Node (Pico W)
    participant Mesh_Node as Mesh Node (Pico W)

    Master_Node ->> Mesh_Node: Connect to AP (SSID: MeshNode)
    Master_Node ->> Mesh_Node: Send TCP Packet "Hello from Master"
    Mesh_Node ->> Master_Node: Receive TCP Packet
    Mesh_Node ->> Master_Node: Reply "Hello from Mesh Node"
    Master_Node ->> Mesh_Node: Continue Data Exchange
```


<br/><br/>

---  

### Initial Whiteboard Sketch
![Whiteboard Sketch](https://github.com/Rackhamn/iot_arch_project1/blob/main/Resources/Screenshot%202025-01-23%20094007.png)
