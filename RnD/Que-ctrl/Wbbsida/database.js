// JavaScript for SQLite database interactions

// Initialize SQLite database
const sqlite3 = require('sqlite3').verbose();
const db = new sqlite3.Database('jats.db', (err) => {
    if (err) {
        console.error('Error opening database ' + err.message);
    } else {
        console.log('Connected to the SQLite database.');
    }
});

// Function to add a user
function addUser (username, name, email, password, callback) {
    if (name && email) {
        // For regular users
        db.run(`INSERT INTO users (username, name, email, password) VALUES (?, ?, ?, ?)`,
        [username, name, email, password], function(err) {
            if (err) {
                return console.error(err.message);
            }
            console.log(`User  ${username} added to the database with ID: ${this.lastID}`);
            if (callback) callback(this.lastID); // Call the callback with the new user ID
        });
    } else {
        // For admin user
        db.run(`INSERT INTO users (username, password) VALUES (?, ?)`,
        [username, password], function(err) {
            if (err) {
                return console.error(err.message);
            }
            console.log(`Admin user ${username} added to the database with ID: ${this.lastID}`);
            if (callback) callback(this.lastID); // Call the callback with the new user ID
        });
    }
}

// Function to check if a user exists
function checkUserExists(username) {
    db.get(`SELECT * FROM users WHERE username = ?`, [username], (err, row) => {
        if (err) {
            return console.error(err.message);
        }
        if (row) {
            console.log(`User  found: ${JSON.stringify(row)}`);
        } else {
            console.log('User  not found.');
        }
    });
}

// Function to add a tag
function addTag(tag_id, name, area, user_id) {
    db.run(`INSERT INTO tags (tag_id, name, area, user_id) VALUES (?, ?, ?, ?)`, [tag_id, name, area, user_id], function(err) {
        if (err) {
            return console.error(err.message);
        }
        console.log(`Tag ${tag_id} added to the database.`);
    });
}

// Ensure database structure is correct before inserting data
db.serialize(() => {
    // Drop the existing users table
    db.run(`DROP TABLE IF EXISTS users`, (err) => {
        if (err) {
            console.error('Error dropping table: ', err.message);
        } else {
            console.log('Users table dropped.');
            
            // Create the users table AFTER dropping
            db.run(`CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL,
                password TEXT NOT NULL,
                email TEXT,
                name TEXT
            )`, (err) => {
                if (err) {
                    console.error('Error creating users table: ', err.message);
                } else {
                    console.log('Users table created.');
                    
                    // Insert users only after the table is created
                    addUser ('admin', null, null, 'admin', (adminId) => {
                        addUser ('emma', 'Emma Smith', 'emmas@hotmail.com', 'emmaPassword', (emmaId) => {
                            addUser ('eva', 'Eva Larsson', 'eval@hotmail.com', 'evaPassword', (evaId) => {
                                addUser ('malin', 'Mailin Vente', 'malinv@hotmail.com', 'malinPassword', (malinId) => {
                                    addUser ('betty', 'Betty Vanderwoodsen', 'bettyw@hotmail.com', 'bettyPassword', (bettyId) => {
                                        // Call the function to check for the admin user
                                        checkUserExists('admin');

                                        // Drop the existing tags table
                                        db.run(`DROP TABLE IF EXISTS tags`, (err) => {
                                            if (err) {
                                                console.error('Error dropping tags table: ', err.message);
                                            } else {
                                                console.log('Tags table dropped.');

                                                // Create the tags table
                                                db.run(`CREATE TABLE IF NOT EXISTS tags (
                                                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                                                    tag_id TEXT NOT NULL UNIQUE,
                                                    name TEXT NOT NULL,
                                                    area TEXT NOT NULL,
                                                    user_id INTEGER NOT NULL,
                                                    FOREIGN KEY (user_id) REFERENCES users(id)
                                                )`, (err) => {
                                                    if (err) {
                                                        console.error('Error creating tags table: ', err.message);
                                                    } else {
                                                        console.log('Tags table created.');

                                                        // Insert sample tags
                                                        addTag('tag001', 'Sample Tag 1', 'Area 1', adminId); // Use adminId
                                                        addTag('tag002', 'Sample Tag 2', 'Area 2', emmaId); // Use emmaId
                                                        addTag('tag003', 'Sample Tag 3', 'Area 3', evaId); // Use evaId
                                                        addTag('tag004', 'Sample Tag 4', 'Area 4', malinId); // Use malinId
                                                        addTag('tag005', 'Sample Tag 5', 'Area 5', bettyId); // Use bettyId
                                                    }
                                                });
                                            }
                                        });
                                    });
                                });
                            });
                        });
                    });
                }
            });
        }
    });
});

// Function to close the database connection
function closeDatabase() {
    db.close((err) => {
        if (err) {
            console.error(err.message);
        }
        console.log('Closed the database connection.');
    });
}

// Export the database and close function if needed
module.exports = {
    db,
    closeDatabase
};
