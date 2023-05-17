#include "mysql_conn.h"

mysql_conn::mysql_conn(const char* user, const char* pwd, const char* db_name){
    this->user = user;
    this->pwd = pwd;
    this->db_name = db_name;
    cout << "construct mysql connection" << endl;
}

mysql_conn::~mysql_conn(){
    mysql_close(&mysql);
    cout << "destroy mysql connection" << endl;
}

bool mysql_conn::initDB(const char* host,const char* user,const char* pwd,const char* db_name, unsigned int port){
    if(mysql_init(&mysql) == NULL){
        cout << "sql init error" << endl;
        throw exception();
        return false;
    }
    MYSQL *res = mysql_real_connect(&mysql, host, user, pwd, db_name, port, "/var/run/mysqld/mysqld.sock", 0);
    if(res == NULL){
        fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(&mysql));
        return false;
    }
    return true;
}

bool mysql_conn::exeSQL(string sql){
    if(mysql_query(&mysql, sql.c_str()) != 0){
        fprintf(stderr, "Failed sql: Error: %s\n", mysql_error(&mysql));
        return false;
    }
    result = mysql_store_result(&mysql);
    return true;
}

vector<vector<string>> mysql_conn::process_data(){
    vector<vector<string>> data;
    if(result){
        int row = mysql_num_rows(result);
        int field = mysql_num_fields(result);
        MYSQL_ROW line = mysql_fetch_row(result);
        string tmp;
        while(line != NULL){
            vector<string> linedata;
            for(int i = 0; i < field; i++){
                tmp = (line[i]) ? line[i] : "NULL";
                linedata.push_back(tmp);
            }
            line = mysql_fetch_row(result);
            data.push_back(linedata);
        }
    }
    return data;
}

void mysql_conn::closeDB(){
    mysql_close(&mysql);
}
