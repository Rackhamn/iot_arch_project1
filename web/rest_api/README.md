## Basic draft for the website / http server REST API

### Questions
```js
// NESTED
GET /api/v1/user/{user_id}/areas
// FLAT + QUERY PARAMS
GET /api/v1/areas?user={user_id}
```
Do we want JSON sent back and forth?  
Do we want the response to be HMTL-text?  

### Examples
  
Let's assume that we have tags, users and areas in different monolithic tables in SQLite.  
  
Fetch the first 20 tags for a logged in user's area  
```js
GET /api/v1/area/{area_id}/tags?offset=0&limit=20 HTTP/1.1
```

Get all areas that user has acces to or owns  
```js
GET /api/v1/user/{user_id}/areas
```

## Cookies
Login via `POST /api/v1/login + json credentials`  
```js
// Create new user
POST /api/v1/users
Content-Type: application/json
{
"username": "blah",
"email": "blah@blah.com",
"password": "secret"
}
// Response
200 OK + Cookie: session_id=123

// Get current user info
GET /api/v1/users/me
credentials: include // Cookie: session_id=123
```

### Some user examples
| Action | HTTP Method | URL |
| :-- | :-- | :-- |
| Create User | `POST` | `/api/v1/users` |
| Get user info | `GET` | `/api/v1/users/{user_id}` |
| Update user info | `PUT/PATCH` | `/api/v1/users/{user_id}` |
| Delete user | `DELETE` | `/api/v1/users/{user_id}` |
| List all users :admin: | `GET` | `/api/v1/users` |


Create new user (create a new resource (user) in the "users" collection)
```js
POST /api/v1/users
Content-Type: application/json
{
  "username": "someguy",
  "email": "some@guy.com",
  "password": "abc123",
  "preferences": {
    "language": "en",
    "dark_mode": true
  }
}
```

Example JS frontend impl.
```js
// Assumes the user is already logged in and has a session cookie
fetch("/api/v1/tags", {
  method: "GET",
  credentials: "include", // ensures cookies are sent
  headers: {
    "Accept": "text/html"
  }
})
.then(response => {
  if (!response.ok) throw new Error("Not authorized or server error");
  return response.text();
})
.then(html => {
  document.getElementById("tagsContainer").innerHTML = html;
})
.catch(err => {
  console.error("Error fetching tags:", err);
});
```
