#define FILENAME_SIZE 50
#define MSJ_SIZE 250
#define USERNAME_SIZE 24


typedef struct{
    int codigo;
    char userName[USERNAME_SIZE];
    int id;
}struct_TCPL;

typedef struct{
    int codigo;
    int userIDSender;
    int userIDReceiver;
    char msj[MSJ_SIZE];
}struct_TMSJ;
