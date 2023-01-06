import socket
import json
from sklearn.naive_bayes import MultinomialNB
import pandas as pd
import pickle
import os
import datetime 


############# INPUT FILE #################
filename = 'sensor_value_used.json'

file_model_predict_water = open('model_predict.pkl', 'rb')
model_predict_water = pickle.load(file_model_predict_water)
file_model_predict_water.close()

######## INITIAL THE VARIABLE #########
 
s = socket.socket()         
 
s.bind(('0.0.0.0', 8090 ))
s.listen(0)       

e = datetime.datetime.now()

today = e.strftime("%d/%m/%Y")
##############################
 
while True:
 
    client, addr = s.accept()
 
    while True:
        content = client.recv(32)
 
        if len(content) ==0:
           break
 
        else:

            x=0

            # print(content)
            print(type(today))

            value_read = content.split()
            float_read = []
            
            for i in value_read:
                float_read.append(float(i))
            

            #INISIALISASI NILAI SETIAP SENSOR DAN SIMPAN DI MASING2 VARIABEL

            tds_value_sensor = float_read[0]
            ph_value_sensor = float_read[1]
            high_value_sensor = float_read[2]

            
            result_predict = model_predict_water.predict([[tds_value_sensor, ph_value_sensor]])[0]
                
            #DEFINISIKAN KE KATEGORI
            if result_predict == 1:
                result_value = "Drinkable"
            elif result_predict==0:
                result_value = "Not Drinkable"
                

            #SAVE DATA TO DICTIONARY
                
            data_obj = {
                "no": x,
                "date": today,
                "solid": tds_value_sensor,
                "ph": ph_value_sensor,
                'high': high_value_sensor,
                'predict': result_value
            }

            if os.path.isfile(filename) and os.access(filename, os.R_OK):
                # checks if file exists
                fp = json.load(open(filename))
                if type(fp) is dict:
                    fp = [fp]
                fp.append(data_obj)  
                with open(filename, 'w') as outfile:
                    json.dump(fp, outfile)
            else:
                with open(filename, 'w') as file_object:
                        json.dump(data_obj, file_object)

            print("tds : ", tds_value_sensor)
            print("ph : ", ph_value_sensor)
            print("high : ", high_value_sensor)
            print("predict : ", result_value)
 
    print("Closing connection")
    client.close()