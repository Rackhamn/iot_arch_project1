import sqlite3

#connect to the database (creating the file if it doesnt't exists already)

db_path = "/home/pi/sensor_data.db"
conn = sqlite3.connect(db_path)
cursor = conn.cursor()

#function for adding the sensorvalues

def insert_sensor_data(sensor_name, value):
    cursor.execute("INSERT INTO sensors (sensor_name, value) VALUES (?, ?)", (sensor_name, value))
    conn.commit()
    
#Exampledata (replace this with values received from Pico W)
insert_sensor_data("Name: ", "Gabriel Carlsson")
insert_sensor_data("Age: ", 25)
# Shut the connection
conn.close()
print("Data has been stored in the database!.")
