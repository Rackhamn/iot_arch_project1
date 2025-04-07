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
