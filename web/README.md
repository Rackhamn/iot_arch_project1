
Server-Website:  
    Create Account  
    Login / Logout + 2Auth?  
    view owned spaces/areas  
    view member of spaces/areas  
  
- User:  
    Create new Area (creator is admin)  
    Leave existing Area  
    Can report bugs or issues with the software/service  
    Can view manuals for RFID reader and tutorials for Website
  
- User as Owner of Area:  
    add users to Area (users as user, mod or admin)  
    remove users from Area  
    update users privledges (user / mod / admin)  
    add/edit/remove RFID tag + info DB listing  
    view analytics over time  
  
- User as Member of Area:  
    Search for tag  
    View tags (partial information)  
    Can log details about a tag (discrepency, opt photo)  
    View list of all area members + contact details (admin&mods on top)  
  
- Area Mesh Network  
    Can connect to Server with UID (Attempts to connect to Server)  
    Can request info from Server  
    Can send info to Server  
    Can be connected to like a Router is (additional functions)  
  
- RFID Interface (Reader)  
    Displays its UID on startup  
    Attempts to connect to Mesh Network  
        Can edit its UID, local network SSID, store hashed password  
    Can read information to the RFID/NFC tag  
    Can store fetched info from server  
    Can display list of fetched tags  
    Can list latest fetched tag (or any in list)  
      
- RFID Interface (Writer)  
    Displays its UID on startup  
    Attempts to connect to Mesh Network  
        Can edit its UID, local network SSID, store hashed password  
    Can write information to the RFID/NFC tag  
    Can store fetched info from server  
    Can display list of fetched tags  
    Can list latest fetched tag (or any in list)  
  
- Server Admin  
    Can list all tags in system  
    Can list all Areas in system  
    Can list all users in system (active and inactive + versioned)  
    Can view all analytics in system  
    Can view all logs in system  
    Can sort all * by   latest activity  
                        most activity  
    Can view all changes done on the DB   
    Can view all public/private keys  
    Can ping a message to any RFID reader  
    Can list all known IP addresses (Routers / Mesh APs)  
    Can list all Mesh APs  
    Can remotely shut down any mesh AP / RFID Interface (kill switch)  
    Can remotely update partially any Mesh AP / RFID Interface  
  
  
A message from the Mesh AP Network to the Server   
will send its own net_UID and the interface_UID.  
if the interface_UID has been added to the whitelist it will be allowed.  
Otherwise the interface will get a response of "N/A Action" or "Error X"  
The owner / mod of an Area have to add the interface_UID to the whitelist   
via the website or directly on the Mesh AP Router page.  
  
Any action done on the Mesh AP Router page will be logged and eventually  
sent up to the server once connected.  
Any action done on the website will be logged as well. (append file in server + SQLite fd* & parity copy)  
  
The server will back up log pages once no more activity has been seen on it after N seconds.  
  
The DB listing for all tags within an Area will have a split description.  
One part for the Admin to view & edit and one part (copy/ref) for the users to view.  
When using the interface the server will only pull the user viewable data.  
Thus the owner/admin data will not be exposed outside of the server.  
  
If a user updates a discrepency or makes a comment about a tag and sends an image / file,   
the file will be placed within a secure container (block, data, block) so that no bad files can be remotely run on the server.  
Each file is placed under the "AreaUID/logs/$User/$Date/X" folder.  
This is done because some files, like JPEGs or PDFs, when opened or decompressed can have malicous code insertions that breaks the reading function.  
We do NOT want anyone to open the files if they are in any way dangerous.  
Not even the operating system itself.  
The cache might have the files in RAM in the format that they came in as.  
  
Any text written into the website WILL be treated as malicious until the Server  
itself has verified and run validity on it. (perhaps even clean it up)  
The frontend does this also - but it CANNOT EVER be seen as reliable.  
  
