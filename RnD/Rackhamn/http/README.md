# HTTP server in C with JSON REST api

This is going to be an HTTP (only) server that serves a website that handles users and request via a REST api through JSON data.  
It is HTTP (only) because of the fact that i DO NOT have a SSL certificate laying around.  
  
  
The plan is to expose the API endpoints in a more dynamic way.  
Maybe there will be a \*.INI-like file where an endpoint api maps to a function with parameters.  
Those functions are loaded in and stored inside a table by version and name (hashed name?).  
When a given API request comes through we can simply call the function from the table.  
  
If this is it not a good option, a strcmp on the URI might be sufficient.  
Altough, at that point, using versioning is gonna be a little annoying but might still work.  
  
We will probably use the following "libraries":  
`RnD/Rackhamn/arena`  
`RnD/Rackhamn/json`  
`Github/Rackhamn/sha256_session_id`  
  
## API Examples

## Client Side HTTP + JS
```http
<body>
  <h2>Update Tag Info</h2>
  
  <label for="uid">Tag UID:</label>
  <input type="text" id="uid" placeholder="Tag UID">

  <label for="description">Description</label>
  <input type="text" id="description" placeholder="Tag Description">

  <label for="username">Username</label>
  <input type="text" id="username" placeholder="Username">

  <button onclick="updateTag()">Update Tag</button>

  <p id="update_tag_result"></p>
</body>
```
```js
async function updateTag(uid, newData) {
    const uid = document.getElementById("uid");
    const description = document.getElementById("description");
    const username = document.getElementById("username");

    if(!uid) {
        document.getElementById("result").innerText = "Error: Tag UID is required!";
        return;
    }

    const data = { uid: uid, description: description, username: username };

    try {
        const response = await fetch("/api/v1/tags/" + uid, {
            method: "PUT",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(data)
        });

        const result = await response.json();

        if (!response.ok) {
            document.getElementById("result").innerText = "Error: ${result.message}";
            return;
        }

        document.getElementById("result").innerText = "Tag updates successfully!";
    } catch (error) {
        document.getElementById("result").innerText = "Network Error?!";
    }
}
```

## Client Login Request
```js
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
```js
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
```js
HTTP/1.1 400 Unauthorized
Content-Type: application/json

{
  "status": "error",
  "message": "Login Failed"
}
```

## Client get tags request
```js
GET /api/v1/tags HTTP/1.1
Host: jats.se
Accept: application/json
Cookie: session_id...
```

### Response (JSON)
```js
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
  ]
}
```

## Client add new tag request
```js
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
```js
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
```js
HTTP/1.1 400 Bad Request
Content-Type: application/json

{
  "status": "error",
  "message": "Tag UID already exists"
}
```


