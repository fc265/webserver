#include<iostream>
#include "mysql_conn.h"
#include <sys/time.h>

using namespace std;

int main(){
    struct timeval timeStart, timeEnd, timeSystemStart; 
    double runTime=0, systemRunTime; 
    gettimeofday(&timeSystemStart, NULL );


    mysql_conn *db = new mysql_conn("root", "123", "webserver");
    db->initDB("localhost", "root", "123", "webserver", 0);
    //string sql = "insert into user_info(uid, name, pwd) values(3, 'my', '13456');";
    string sql = "select pwd from user_info where name='fc';";
    //string sql = "delete from user_info where uid=2;";
    gettimeofday(&timeStart, NULL );
    if(db->exeSQL(sql)){
        vector<vector<string>> data = db->process_data();
        for(auto line : data){
            for(auto s : line) cout << s << endl;
            cout << endl;
        }
        delete(db);
    }
    gettimeofday( &timeEnd, NULL ); 
    runTime = (timeEnd.tv_sec - timeStart.tv_sec ) + (double)(timeEnd.tv_usec -timeStart.tv_usec)/1000000;  
    systemRunTime = (timeEnd.tv_sec - timeSystemStart.tv_sec ) + (double)(timeEnd.tv_usec -timeSystemStart.tv_usec)/1000000;    
    printf("runTime is %lf\n", runTime);    
    printf("systemRunTime is %lf\n", systemRunTime);
    
    
    
}