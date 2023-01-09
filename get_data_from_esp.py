import socket
import json
import pickle
import os
import datetime 



############# INPUT FILE #################
filename = 'sensor_value_used.json'

file_model_predict_water = open('model_predict.pkl', 'rb')
model_predict_water = pickle.load(file_model_predict_water)
file_model_predict_water.close()

######## INITIAL THE VARIABLE #########
 
# importing the libraries
from bs4 import BeautifulSoup
import requests

url="http://192.168.50.192/"

time = datetime.datetime.now()

today = time.strftime("%d/%m/%Y")

x=0
while(1):

    # Make a GET request to fetch the raw HTML content
    html_content = requests.get(url).text
    # Parse the html content
    soup = BeautifulSoup(html_content, "lxml")
    #print(soup.prettify()) # print the parsed data of html
    
    high_value_sensor=float(soup.h4.text)
    ph_value_sensor=float(soup.h3.text)
    tds_value_sensor=float(soup.h2.text)

    result_predict=model_predict_water.predict([[tds_value_sensor, ph_value_sensor]])[0]

    if result_predict ==1:
        result_value="Drinkable"
    elif result_predict == 0:
        result_value="Not Drinkable"
    

    data_obj ={
        "date" : today,
        "solid": tds_value_sensor,
        "ph": ph_value_sensor,
        "high":high_value_sensor,
        "predict": result_value
    }

    if os.path.isfile(filename) and os.access(filename, os.R_OK):
        fp = json.load(open(filename))
        if type(fp) is dict:
            fp = [fp]
        fp.append(data_obj)
        with open(filename, 'w') as outfile:
            json.dump(fp, outfile)
    else:
        with open(filename, 'w') as file_object:
            json.dump(data_obj, file_object)
    

    print("Nilai TDS : ", tds_value_sensor)
    print("Nilai PH : ", ph_value_sensor)
    print("Nilai HC : ", high_value_sensor)import socket
import json
import pickle
import os
import datetime 



############# INPUT FILE #################
filename = 'sensor_value_used.json'

file_model_predict_water = open('model_predict.pkl', 'rb')
model_predict_water = pickle.load(file_model_predict_water)
file_model_predict_water.close()

######## INITIAL THE VARIABLE #########
 
# importing the libraries
from bs4 import BeautifulSoup
import requests

url="http://192.168.50.192/"

time = datetime.datetime.now()

today = time.strftime("%d/%m/%Y")

x=0
while(1):

    # Make a GET request to fetch the raw HTML content
    html_content = requests.get(url).text
    # Parse the html content
    soup = BeautifulSoup(html_content, "lxml")
    #print(soup.prettify()) # print the parsed data of html
    
    high_value_sensor=float(soup.h4.text)
    ph_value_sensor=float(soup.h3.text)
    tds_value_sensor=float(soup.h2.text)

    result_predict=model_predict_water.predict([[tds_value_sensor, ph_value_sensor]])[0]

    if result_predict ==1:
        result_value="Drinkable"
    elif result_predict == 0:
        result_value="Not Drinkable"
    

    data_obj ={
        "date" : today,
        "solid": tds_value_sensor,
        "ph": ph_value_sensor,
        "high":high_value_sensor,
        "predict": result_value
    }

    if os.path.isfile(filename) and os.access(filename, os.R_OK):
        fp = json.load(open(filename))
        if type(fp) is dict:
            fp = [fp]
        fp.append(data_obj)
        with open(filename, 'w') as outfile:
            json.dump(fp, outfile)
    else:
        with open(filename, 'w') as file_object:
            json.dump(data_obj, file_object)
    

    print("Nilai TDS : ", tds_value_sensor)
    print("Nilai PH : ", ph_value_sensor)
    print("Nilai HC : ", high_value_sensor)