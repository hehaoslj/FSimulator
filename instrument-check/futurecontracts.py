# -*- coding:utf-8 -*-
#! C:/Anaconda3/python

import urllib.request as ur
import pandas as pd
from pandas import DataFrame
import numpy as np
import json
import datetime
import os
import time
import socket



'''
all the data here is from sina.com.
realtime data from :   http://hq.sinajs.cn/list='code'
historiacl data from :  stock2.finance.sina.com.cn/futures/api/json.php/IndexService.getInnerFuturesDailyKLine?symbol='code'
'''
def GetVolume(datestr,contract):
    url = 'http://stock2.finance.sina.com.cn/futures/api/json.php/IndexService.getInnerFuturesDailyKLine?symbol='+contract;
    #time.sleep(1);
    
    flag = True;
    while flag== True:
        try:
            data = ur.urlopen(url).read().decode('utf-8');
            flag = False;
            
        except :
            time.sleep(5);
            continue;
    df = json.loads(data);
    temp = pd.DataFrame(df,columns=['Date','Open','High','Low','Close','Volume']);
    exactline = temp[temp.Date == datestr];#get the exactline
    if len(exactline)==1:
        if type(exactline.iloc[0].Volume)==int:
            return exactline.iloc[0].Volume;
        elif type(exactline.iloc[0].Volume)==str:
            return int(exactline.iloc[0].Volume);
        else:
            print('something wrong with the value of Volume, neither int nor str!');
            return ;
    elif len(exactline)>1:
        print('Please check Data from Sina,there is more than one day-k for a single day!');
        return ;
    else:
        print('No data for this day or Parament error, please check again!');
        return;

socket.setdefaulttimeout(5);

startdate = datetime.datetime.strptime('2016-09-01','%Y-%m-%d').date();
enddate = datetime.date.today()-datetime.timedelta(1);
#print(startdate);
#print(enddate);

contracts = ['HC','RB'];# change the contracts here, you'd better delelte the old file
deliverymonth = []
#delivery month should be calculated as current year + 2 year
rightnow = datetime.datetime.now();
thisyear = rightnow.year;
thismonth = rightnow.month;

startyear = '16';
endyear = '17'# if you change the contracts range, all the data existed means nothing. please delete the old file or point to another pisiton.

for i in range(int(startyear),int(endyear)+1):#if more contracts needed, adjust the range here to have more years!
    for n in range(1,13):
        if(n<10):
            deliverymonth.append(str(i)+'0'+str(n));
        else:
            deliverymonth.append(str(i)+str(n));
#deliverymonth contains all months from 16-18
#print(deliverymonth);
filedest = 'D:/Data/HC0102/Contracts.csv';#set the destination file route
filecache = pd.DataFrame();
print(filecache);

blackboard = {};# a temprary dict to save the contracts details
for i in contracts:
    for n in range(0,3):
        blackboard[i+'0'+str(n)] = '';# initiate the temp dict
#print(blackboard);       
        

if os.path.exists(filedest):
    filecache = pd.read_csv(filedest,names=['Date','Contracts','Instrument'],header=0,);    
    if(len(filecache)!=0):
        datestrtemphead = filecache.head(1).iloc[0][0];
        datestrtemptail = filecache.tail(1).iloc[0][0];#the lateset date: get the str
        print(datestrtemphead,datestrtemptail);
        datehead = datetime.datetime.strptime(datestrtemphead,'%Y-%m-%d').date();
        datetail = datetime.datetime.strptime(datestrtemptail,'%Y-%m-%d').date()
        print('startdate is:',startdate);
        print('enddate is:',enddate);
        if startdate < datehead:
            #delete the file and start from the startdate
            os.remove(filedest);
            filecache = pd.DataFrame();
            datepointer = startdate;
        else:
            #filecache = pd.read_csv(filedest,names=['Date','Contracts','Instrument'],header=0,);
            print(filecache);
            datepointer = datetail + datetime.timedelta(1);#convert the date str into Datetime.Date object
            df = filecache[filecache.Date == datestrtemptail];
            print(df);
            for i in range(0,len(df)-1):
                blackboard[df.iloc[i][1]]=df.iloc[i][2];
            print(blackboard);
    else:
        os.remove(filedest);
        datepointer = startdate;
        pass;
else:
    datepointer = startdate;#we will start from the very startdate

#print(datepointer);
#print(datepointer <= enddate);
#print(filecache);

while datepointer <= enddate:
    #with the datepointer moves from startdate to enddate, here compute the contract strength of all category
    datepointer_str = datepointer.strftime('%Y-%m-%d');
    print(datepointer_str);
    for con in contracts:
        conboss = blackboard[con+'00'];
        conmateafter = blackboard[con+'01'];
        conmatebefore = blackboard[con+'02'];
        if conboss !='':
            conbossV = GetVolume(datepointer_str,conboss);
            if conbossV == None:
                print('comes here!');
                break; #current datepointer has no data, break the for-loop to move forward;
        else:
            conbossV = 0 ;
        conmateafterV = 0;
        conmatebeforeV = 0;
        print(con);
        print(conboss,conbossV);
        print(conmateafter,conmateafterV);
        print(conmatebefore,conmatebeforeV);
                   
        for dlv in deliverymonth:
            #print(dlv);
            instru =con+dlv;# should be like 'HC1610'
            print(instru);
            
            url = 'http://stock2.finance.sina.com.cn/futures/api/json.php/IndexService.getInnerFuturesDailyKLine?symbol='+instru;
            flag = True;
            while flag== True:
                try:
                    data = ur.urlopen(url).read().decode('utf-8');
                    flag = False;
                    
                except:
                    time.sleep(5);
                    continue;
                
            '''
            data = ur.urlopen(url).read().decode('utf-8');
            time.sleep(1);'''
            df = json.loads(data);
            temp = pd.DataFrame(df,columns=['Date','Open','High','Low','Close','Volume']);
            #print(GetVolume(datepointer_str,instru));
            #print(temp[:1]);
            #datepointer_str = datepointer.strftime('%Y-%m-%d');
            
            if(len(temp[temp['Date'].isin([datepointer_str])])!=0):
                if instru > conboss:
                    mover = GetVolume(datepointer_str,instru);
                    if mover >= conbossV:
                        conboss = instru;
                        conbossV = mover;
                        blackboard[con+'00'] = instru;
                        print('find the boss!');
                    else:
                        pass;
                else:
                    pass;
            else:
                pass;
        
        #here we will update the second contract befre and after the updated mail contract - conboss
        for dlv in deliverymonth:
            instru =con+dlv;# should be like 'HC1610'
            print(instru,'finding mate here');
            
            url = 'http://stock2.finance.sina.com.cn/futures/api/json.php/IndexService.getInnerFuturesDailyKLine?symbol='+instru;

            flag = True;
            while flag== True:
                try:
                    data = ur.urlopen(url).read().decode('utf-8');
                    flag = False;
                    #sleep(1);
                except:
                    time.sleep(5);
                    continue;
            
            #data = ur.urlopen(url).read().decode('utf-8');
            df = json.loads(data);
            temp = pd.DataFrame(df,columns=['Date','Open','High','Low','Close','Volume']);
            #datepointer_str = datepointer.strftime('%Y-%m-%d');
            
            if(len(temp[temp['Date'].isin([datepointer_str])])!=0):
                mover = GetVolume(datepointer_str,instru);
                
                if instru > conboss:
                    print(mover,' vs ', conmateafterV);
                    if (mover >= conmateafterV) & (mover != 0):
                        conmateafter = instru;
                        conmateafterV = mover;
                        blackboard[con+'01'] = instru;
                        print('found the aftermate!');
                    else:
                        pass;
                elif instru < conboss:
                    print(mover,' vs ', conmatebeforeV);
                    if (mover >= conmatebeforeV) & (mover !=0):
                        conmatebefore = instru;
                        conmatebeforeV = mover;
                        blackboard[con+'02'] = instru;
                        print('found the beforemate!');
                    else:
                        pass;
                else:
                    #main contract itself is coming
                    pass;
            else:
                pass;

    print(blackboard);#deal the blackboard here;
    
    c=[];
    for (i,v) in blackboard.items():
        temp = [datepointer_str,i,v];
        c.append(temp);
    dftemp=pd.DataFrame(c,columns=['Date','Contracts','Instrument']);
    if len(filecache) == 0:
        filecache = dftemp.copy();
    else:
        filecache=pd.concat([filecache,dftemp],ignore_index=True);

    filecache.sort(columns=['Date']);
    print(filecache);

    print(datepointer);
    datepointer= datepointer+datetime.timedelta(1);


if len(filecache)!=0:
    filecache.to_csv(filedest,names=['Date','Contracts','Instrument']);
    pass;
else:
    pass;

'''
#today = datetime.date.today()+datetime.timedelta(50);

#print( startdate+datetime.timedelta(70) == enddate );

url = 'http://stock2.finance.sina.com.cn/futures/api/json.php/IndexService.getInnerFuturesDailyKLine?symbol=HC1701';

data = ur.urlopen(url).read().decode('utf-8');

df = json.loads(data);
#print(type(df[0][0]));
temp = pd.DataFrame(df,columns=['Date','Open','High','Low','Close','Volume']);
#convert the str data to a list ;
#print(type(temp.Volume));
#print(int(temp[temp.Date == '2016-01-19'].Volume));
#print(temp.loc[temp.Date=='2016-01-19'].Volume);
'''



