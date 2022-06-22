
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


// se usa en el caso de querer saber is un usuario existe, no debe usarse para buscar por id, sino por userName

//respuesta: codigo = 0: el usuario esta conectado
// codigo = -1  el usuario no esta conectado pero si registrado
// codigo = -2 el usuario no esta registrado
typedef struct{
    char codop[4];//UDPS
    int codigo;
    char userName[24];
    int id;
}struct_cliente_UDPS;

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

//se envia con los campos setteados: codop, userIDSender, userIDReceiver, msj

// el servidor responde con un mensaje de tipo TMSR

typedef struct{
    char codop[4]; //TMSJ
    int codigo;
    int userIDSender;
    int userIDReceiver;
    char msj[MSJ_SIZE];
}struct_cliente_TMSJ;

//Respuesta al envio de un mensaje, nos notifica si se entregó correctamente o no

typedef struct{
    char codop[4];
    int codigo;
    int userIDSender;
}__attribute__((packed)) struct_cliente_TMSR;


//se usa para enviar un archivo a otro usuario, los campos setteados son: codop, userIDSender, userIDReceiver, fileName, fileSize;

//la respuesta del servidor, cuando está listo para enviar el archivo, es este mismo struct pero con el campo codigo setteado a 0

typedef struct{
    char codop[4];
    int codigo;
    int userIDSender;
    int userIDReceiver;
    char fileName[FILENAME_SIZE];
    long int fileSize;
}__attribute__((packed)) struct_cliente_TCFS ;
