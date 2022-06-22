// en ninguno se tiene en cuenta el codop, que viene antes y es char[4]

#define FILENAME_SIZE 50
#define MSJ_SIZE 250
#define USERNAME_SIZE 24


//El cliente envia el paquete con el campo codop especificado y el nombre de usuario setteado, el servidor responde con el mismo registro pero con el ID setteado y el codigo de error en 0 si está todo bien, -1 en cualquier otro caso

typedef struct{
    char codop[4];//UDPR
    int codigo;
    char userName[24];
    int id;
}struct_cliente_UDPR;

typedef struct{
    char codop[4];//UDPC
    int codigo;
    int cantidadLogueados;
    int cantidadRegistrados;
}struct_cliente_UDPC;

//el cliente envía los datos de su usuario y el servidor responde el paquete con codigo == 0 en el caso de que esté todo bien

typedef struct{
    char codop[4];//TCPL
    int codigo;
    char userName[USERNAME_SIZE];
    int id;
}struct_cliente_TCPL;

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


typedef struct{
    char codop[4];
    int codigo;
    int userIDSender;
    int userIDReceiver;
    char msj[MSJ_SIZE];
}struct_cliente_TMSJ;

typedef struct{
    char codop[4];
    int codigo;
    int userIDSender;
}__attribute__((packed)) struct_cliente_TMSR;


typedef struct{
    char codop[4];
    int codigo;
    int userIDSender;
    int userIDReceiver;
    char fileName[FILENAME_SIZE];
    long int fileSize;
}__attribute__((packed)) struct_cliente_TCFS ;
