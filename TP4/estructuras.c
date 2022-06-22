struct_cliente_UDPR unpackUDPR(char buf[]){
    struct_cliente_UDPR *miStruct;
    memset(&miStruct, 0, sizeof(miStruct));   /* Zero out structure */

    miStruct = (struct_cliente_UDPR *)buf;

    return *miStruct;
}

struct_cliente_UDPS unpackUDPS(char buf[]){
    struct_cliente_UDPS *miStruct;
    memset(&miStruct, 0, sizeof(miStruct));   /* Zero out structure */

    miStruct = (struct_cliente_UDPS *)buf;

    return *miStruct;
}
