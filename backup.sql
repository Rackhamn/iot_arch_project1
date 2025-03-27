PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE rfid_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    rfid_tag TEXT NOT NULL,
    location TEXT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
);
INSERT INTO rfid_data VALUES(1,'ABC123','Lagerhylla A1','2025-03-25 23:57:59');
CREATE TABLE sensor_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    rfid_tag TEXT NOT NULL,
    sensor_value REAL NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (rfid_tag) REFERENCES rfid_data(rfid_tag)
);
INSERT INTO sensor_data VALUES(1,'ABC123',22.5,'2025-03-25 23:57:59');
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('rfid_data',1);
INSERT INTO sqlite_sequence VALUES('sensor_data',1);
COMMIT;
