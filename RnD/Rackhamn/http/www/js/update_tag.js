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

        document.getElementById("result").innerText = "Tag updates successfully!";
    } catch (error) {
        document.getElementById("result").innerText = "Network Error?!";
    }
}
