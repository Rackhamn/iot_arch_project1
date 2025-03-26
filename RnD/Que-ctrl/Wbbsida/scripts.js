// JavaScript for handling user interactions
console.log('Script loaded!');
// Function to handle login
document.getElementById('loginForm').addEventListener('submit', function(event) {
    event.preventDefault();
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;

    // Make a request to the server to log in
    fetch('/login', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ username, password })
    })
    .then(response => {
        if (response.ok) {
            return response.json();
        } else {
            throw new Error('Invalid credentials');
        }
    })
    .then(data => {
        // Store the username in localStorage
        localStorage.setItem('username', data.username);

        
        // Check if the user is an admin
        if (data.username === 'admin') {
            // Redirect to admin.html for admin users
            window.location.href = 'admin.html';
        } else {

            // Redirect to dashboard.html for normal users
            window.location.href = 'dashboard.html';
        }
    })
    .catch(error => {
        alert(error.message);
    });
});
// make this work
// Function to display the username on the dashboard
document.addEventListener('DOMContentLoaded', function() {
    const userName = localStorage.getItem('username');
    if (userName) {
        document.getElementById('userName').innerText = userName;
    } else {
        console.log('No username found in localStorage'); // Log if no username is found
    }
});

// Function to toggle the hamburger menu
function toggleHamburgerMenu() {
    const menu = document.getElementById('hamburgerMenu');
    menu.style.display = menu.style.display === 'block' ? 'none' : 'block';
}

window.onclick = function(event) {
    const menu = document.getElementById('hamburgerMenu');
    const hamburgerButton = document.querySelector('.hamburger-icon');

    // Check if the click was outside the menu and the hamburger button
    if (!menu.contains(event.target) && !hamburgerButton.contains(event.target)) {
        menu.style.display = 'none'; // Hide the menu
    }
};

function logout() {
    console.log('Logout function called'); // Debug log
    localStorage.removeItem('username'); // Remove the username from localStorage
    window.location.href = 'index.html'; // Redirect to the login page or home page
}

// Function to fetch and display all users
function fetchUsers() {
    fetch('/users')
        .then(response => response.json())
        .then(users => {
            const userList = document.getElementById('userList');
            userList.innerHTML = ''; // Clear existing list
            users.forEach(user => {
                const li = document.createElement('li');
                li.textContent = `${user.username} (${user.email})`;
                userList.appendChild(li);
            });
        })
        .catch(error => console.error('Error fetching users:', error));
}


// Function to handle form submisson
document.getElementById('createUserForm').addEventListener('submit', function(event) {
    event.preventDefault(); // Prevent the default form submission
    const username = document.getElementById('newUsername').value;
    const password = document.getElementById('newPassword').value;
    const email = document.getElementById('newEmail').value;

    console.log('Creating user:', username, email); // Log the user details

    fetch('/create-user', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ username, password, email })
    })
    .then(response => {
        console.log('Response received:', response); // Log the response
        if (response.ok) {
            alert('User created successfully!');
            fetchUsers(); // Refresh the user list
        } else {
            throw new Error('Failed to create user');
        }
    })
    .catch(error => alert(error.message));
});

// Function to log out
function logout() {
    localStorage.removeItem('username');
    window.location.href = 'index.html';
}

// Placeholder functions for other actions
function showRegisteredTags() {
    // Logic to show registered tags
}

// Function to show users
function showUsers() {
    document.getElementById('userManagement').style.display = 'block'; // Show user management section
    document.getElementById('userManagementList').style.display = 'none'; // Hide user management list
    const content = document.getElementById('content');
    content.innerHTML = ''; // Clear previous content


    

}
//admin
function showHistory() {
    document.getElementById('userManagement').style.display = 'none'; // Hide user management section
    document.getElementById('userManagementList').style.display = 'none'; // Hide user management list

    // Clear the content area to remove any previously displayed text
    const content = document.getElementById('content');
    content.innerHTML = ''; // Clear previous content

    // Create a new heading for "History"
    const historyHeading = document.createElement('h2');
    historyHeading.textContent = 'History'; // Set the text to "History"
    historyHeading.className = 'history-heading'; // Add a class for styling

    // Append the heading to the content area
    content.appendChild(historyHeading);
}

function showRegisterTag() {
    // Logic to show register tag form
}
//admin
function showAreas() {
    document.getElementById('userManagement').style.display = 'none'; // Hide user management section
    document.getElementById('userManagementList').style.display = 'none'; // Hide user management list

    // Clear the content area to remove any previously displayed text
    const content = document.getElementById('content');
    content.innerHTML = ''; // Clear previous content

    // Create a new heading for "Areas"
    const areasHeading = document.createElement('h2');
    areasHeading.textContent = 'Areas'; // Set the text to "Areas"
    areasHeading.className = 'areas-heading'; // Add a class for styling

    // Append the heading to the content area
    content.appendChild(areasHeading);
}

function showAboutMe() {
    // Logic to show about me information
}

function showJATSInfo() {
    window.location.href = 'jats-info.html'; // Redirect to the JATS Info page
}
//admin
function showTags() {
    document.getElementById('userManagement').style.display = 'none'; // Hide user management section
    document.getElementById('userManagementList').style.display = 'none'; // Hide user management list

    // Create a new section for displaying the "Tags" text
    const content = document.getElementById('content');
    content.innerHTML = ''; // Clear previous content

    // Create a new heading for "Tags"
    const tagsHeading = document.createElement('h2');
    tagsHeading.textContent = 'Tags'; // Set the text to "Tags"
    tagsHeading.className = 'tags-heading'; // Add a class for styling

    // Append the heading to the content area
    content.appendChild(tagsHeading);
}
//admin
// Show the form for creating a new user
document.getElementById('createNewUserBtn').addEventListener('click', function () {
    console.log('Create New User button clicked'); // Debug log
    document.getElementById('userManagementList').style.display = 'block'; // Show user management list
    document.getElementById('createUserForm').style.display = 'block'; // Show the form
});
//admin
document.getElementById('hamburgerMenu').querySelector('button[onclick="showJATSInfo()"]').addEventListener('click', function() {
    window.location.href = 'jats-info.html'; // Redirect to the JATS Info page
});
//does not work
document.getElementById('backButton').addEventListener('click', function() {
    console.log('Back button clicked'); // Debug log
    const username = localStorage.getItem('username');
    if (username === 'admin') {
        window.location.href = 'admin.html'; // Redirect to admin page
    } else {
        window.location.href = 'dashboard.html'; // Redirect to dashboard page
    }
});
//users
function showRegisteredTags() {
    // Clear the content area to remove any previously displayed text
    const content = document.getElementById('content');
    content.innerHTML = ''; // Clear previous content

    // Create a new heading for "Registered Tags"
    const registeredTagsHeading = document.createElement('h2');
    registeredTagsHeading.textContent = 'Registered Tags'; // Set the text to "Registered Tags"
    registeredTagsHeading.className = 'registered-tags-heading'; // Add a class for styling

    // Append the heading to the content area
    content.appendChild(registeredTagsHeading);
}

function showRegisterTag() {
    // Clear the content area to remove any previously displayed text
    const content = document.getElementById('content');
    content.innerHTML = ''; // Clear previous content

    // Create a new heading for "Register Tag"
    const registerTagHeading = document.createElement('h2');
    registerTagHeading.textContent = 'Register Tag'; // Set the text to "Register Tag"
    registerTagHeading.className = 'register-tag-heading'; // Add a class for styling

    // Append the heading to the content area
    content.appendChild(registerTagHeading);
}

function showMyAreas() {
    // Clear the content area to remove any previously displayed text
    const content = document.getElementById('content');
    content.innerHTML = ''; // Clear previous content

    // Create a new heading for "My Areas"
    const myAreasHeading = document.createElement('h2');
    myAreasHeading.textContent = 'My Areas'; // Set the text to "My Areas"
    myAreasHeading.className = 'my-areas-heading'; // Add a class for styling

    // Append the heading to the content area
    content.appendChild(myAreasHeading);
}

createUserBtn.addEventListener('click', function() {
    alert("Button clicked!");
});









