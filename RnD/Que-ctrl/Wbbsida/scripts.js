// JavaScript for handling user interactions

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

// Function to display the username on the dashboard
const userName = localStorage.getItem('username');
if (userName) {
    document.getElementById('userName').innerText = userName;
}

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

// Function to handle creating a new user
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

function showRegisterTag() {
    // Logic to show register tag form
}

function showMyAreas() {
    // Logic to show user's areas
}

function showAboutMe() {
    // Logic to show about me information
}

function showJATSInfo() {
    window.location.href = 'jats-info.html'; // Redirect to the JATS Info page
}

function showTags() {
    // Logic to show all tags for admin
}

// Show users function
function showUsers() {
    document.getElementById('userManagement').style.display = 'block'; // Show user management section
    document.getElementById('userManagementList').style.display = 'none'; // Hide user management list initially
    document.getElementById("createUserSection").style.display = "none"; // Hide form initially
}

document.getElementById('createNewUserBtn').addEventListener('click', function () {
    console.log('Create New User button clicked'); // Debug log
    document.getElementById('createUserSection').style.display = 'block'; // Show the form
    document.getElementById('userManagementList').style.display = 'none'; // Hide user list
});

document.getElementById('hamburgerMenu').querySelector('button[onclick="showJATSInfo()"]').addEventListener('click', function() {
    window.location.href = 'jats-info.html'; // Redirect to the JATS Info page
});

document.getElementById('backButton').addEventListener('click', function() {
    console.log('Back button clicked'); // Debug log
    const username = localStorage.getItem('username');
    if (username === 'admin') {
        window.location.href = 'admin.html'; // Redirect to admin page
    } else {
        window.location.href = 'dashboard.html'; // Redirect to dashboard page
    }
});





