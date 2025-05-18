function insertAfter(node, ref) {
	ref.parentNode.insertBefore(node, ref.nextSibling);
}

async function updateTag(uid, newData) {
    const xuid = document.getElementById("uid");
    const description = document.getElementById("description");
    const username = document.getElementById("username");

    if(!xuid) {
        document.getElementById("result").innerText = "Error: Tag UID is required!";
        return;
    }

    const data = { 
	    uid: xuid.value, 
	    description: description.value, 
	    username: username.value
    };

    try {
        const response = await fetch("/api/v1/tags/", {
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

	result_text = "";
        if(result.data.img !== undefined) {
		result_text += "<img href=\"http://localhost:8082" + result.data.img + "\">";
		var img = document.createElement("img");
		img.src = result.data.img;
		img.alt = "cat black";

		insertAfter(img, document.getElementById("result"));

	} else {
		// result_text += "Tag updated successfully!";
	}
	document.getElementById("result").innerText = "Tag updated successfully!";
    } catch (error) {
        document.getElementById("result").innerText = "Network Error?!";
    }
}

async function Login() {
	const xusername = document.getElementById("username");
	const xpassword = document.getElementById("password");

	if(!xusername) {
		document.getElementById("result").innerText = "Error: Username is required!";
		return;
	}

	if(!xpassword) {
		document.getElementById("result").innerText = "Error: Password is required!";
		return;
	}

	const data = {
		username: xusername.value,
		password: xpassword.value
	};

	try {
		const response = await fetch("/api/v1/login", {
			method: "PUT",
			headers: {
				"Content-Type": "application/json"
			},
			body: JSON.stringify(data)
		});

		const result = await response.json();

		if(!response.ok) {
			document.getElementById("result").innerText = "Error: ${result.message}";
			return;
		}

		result_text = "";
		// update text and jump to logged in / userpage -> from json
		window.location.href = result.redirect_location; // "/mypage.html";

		document.getElementById("result").innerText = "Login Success!";
	} catch(error) {
		// TODO: handle error!
		document.getElementById("result").innerText = error;
		// document.getElementById("result").innerText = "Login Failed!";
	}
}

async function LoadUsername() {
	// get username of logged in user from client
	// put username into id=usertag
	
	const xusertag = document.getElementById("usertag");
	if(!xusertag) {
		// somethings wrong - it aint there
		return;
	}

	xusertag.innerText = "await data";

	var obj = new Object;
	// obj.thing = "haha";
	// obj.get = ["username"];
	obj.get = "username";
	/*
	const data = {
		thing: "username"
	};
	*/
	try {
		const response = await fetch("/api/v1/user", {
			method: "POST",
			headers: {
				"Content-Type": "application/json"
			},
			body: JSON.stringify(obj)
		});

		const result = await response.json();
		/*
		if(!result.ok) { 
			xusertag.innerText = "result not ok!";
			return; 
		}
		*/

		xusertag.innerText = "Welcome, " + result.username;
	} catch(error) {
		xusertag.innerText = error; // "try-catch error";
	}

	// xusertag.innerText = "fuck";
}
