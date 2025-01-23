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
* Denize S. (QueDe)

# Project Information
Designing a system assisting housekeeping and supervisors with post-checkout control for short-term apartment rentals.
  
## Plan
Meeting 1: 2025-01-21, 17:00 _done_
More meeting times etc...
  
## Details
Plans and details to follow
  
## Resources
[Link Example](http://google.com)  
[Github .md helper](https://gist.github.com/allysonsilva/85fff14a22bbdf55485be947566cc09e)  

```mermaid
graph TD
    Server[Server] -->|TCP/IPV4/6| Box
    Server --> DB[Database]
    Box --> A1[a]
    Box --> A2[a]
    Box --> A3[a]
    Box --> P1[P]
    Box --> P2[P]
    A1 --> P1
    A1 --> P2
    A2 --> P1
    A2 --> P2
    A3 --> P1
    A3 --> P2
    subgraph lägenheten
        Box
        A1
        A2
        A3
        P1
        P2
    end
```

### Initial Whiteboard Sketch
![Whiteboard Sketch](https://github.com/Rackhamn/iot_arch_project1/blob/main/Resources/Screenshot%202025-01-23%20094007.png)
