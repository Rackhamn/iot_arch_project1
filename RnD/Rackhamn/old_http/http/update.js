let prev_tagId = {};
let prev_desc = {};

function update_text(element) {
	element.innerText = "Haha";
}

function checkEnter(event, element) {
	// console.log(event.key);
        if (event.key === "Enter") {
            event.preventDefault(); // Prevent new line in contenteditable
            element.blur(); // Trigger blur event
        }
    }

function recordTag(element) {
	const tagId = element.getAttribute("data-id");
	const desc = element.innerText.trim();
	
	prev_tagId = tagId;
	prev_desc = desc;
}

function updateTag(element) {
    const tagId = element.getAttribute("data-id");
    const newDesc = element.innerText.trim();

	if(prev_tagId == tagId && prev_desc == newDesc) {
		// no change at all, drop it
		return;
	}

    // Custom format: [id, desc]
    const bodyData = `update_tag=[${tagId}, ${newDesc}]`;

    fetch("/update_tag", {
        method: "POST", 
        headers: {
            "Content-Type": "text/plain"  // Custom format (not JSON)
        },
        body: bodyData
    })
    .then(response => response.text())
    .then(data => console.log("Updated:", data))
    .catch(error => console.error("Error:", error));
}

function updateUser(element) {
	const newUser = element.innerText.trim();

	const bodyData = `update_user=[${newUser}]`;

	fetch("/update_user", {
		method: "POST",
		headers: {
			"Content-Type": "text/plain"
		},
		body: bodyData
	})
	.then(response => response.text())
	.then(data => load_tags(data))// console.log("User:", data))
	.catch(error => console.error("Error:", error));
}

function load_tags(data) {
	// user username to load them pls
	// repl tagtable
	const tagId = document.getElementById("tagtable");
	if(tagId) {
		tagId.innerHTML = data;
	}
}
