# HTTP server in C with JSON REST api

This is going to be an HTTP (only) server that serves a website that handles users and request via a REST api through JSON data.  
It is HTTP (only) because of the fact that i DO NOT have a SSL certificate laying around.  
  
  
The plan is to expose the API endpoints in a more dynamic way.  
Maybe there will be a \*.INI-like file where an endpoint api maps to a function with parameters.  
Those functions are loaded in and stored inside a table by version and name (hashed name?).  
When a given API request comes through we can simply call the function from the table.  
  
If this is it not a good option, a strcmp on the URI might be sufficient.  
Altough, at that point, using versioning is gonna be a little annoying but might still work.  

## API Examples

## Client Login Request
```json
POST /api/v1/login HTTP/1.1
Host: jats.se
Content-Type: application/json
Accept: application/json
Content-Length: 57


{
  "username": "bob",
  "password": "hashed_password"
}
```

### Response (Success)
```
HTTP/1.1 200 OK
Set-Cookie: session_id=whatever123;
HttpOnly;
Secure;
SameSite=Strict
Content-Type: application/json


{
  "status": "success",
  "message": "Login Successful",
  "user": {
    "id": 99,
    "username": "bob"
  }
}
```

### Response (Failed)
```
HTTP/1.1 400 Unauthorized
Content-Type: application/json

{
  "status": "error",
  "message": "Login Failed"
}
```

## Client get tags request
```
GET /api/v1/tags HTTP/1.1
Host: jats.se
Accept: application/json
Cookie: session_id...
```

### Response (JSON)
```
{
  "tags": [
    {
      "uid": "A1E1B2C4",
      "description": "Dinner Plates, 28cm, white, 4pcs",
      "user_owner": "Bob"
    },
    {
      "uid": "B4A1C3DD",
      "description": "Merlin Painting, 120x260cm, name=\"Knight and Devil\"",
      "user_owner": "Bob"
    }
}
```

## Client add new tag request
```
POST /api/v1/tags HTTP/1.1
Host: jats.se
Content-Type: application/json
Accept: application/json
Content-Length: XXX


{
  "uid": "AABBCCDD",
  "description": "Bluetooth Speaker",
  "user_owner": "Bob",
  "area_name": "Living Room"
}
```

### Response (Success)
```
HTTP/1.1 201 Created
Content-Type: application/json

{
  "status": "success",
  "message": "Tag added successfully",
  "tag": {
    "uid": "AABBCCDD",
    "description": "Bluetooth Speaker",
    "user_owner": "Bob",
    "area_name": "Living Room"
  }
}
```

### Response (Failed)
```
HTTP/1.1 400 Bad Request
Content-Type: application/json

{
  "status": "error",
  "message": "Tag UID already exists"
}
```


