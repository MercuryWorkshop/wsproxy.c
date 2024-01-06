int main(){
    unsigned char frame[10];
    frame[0] = (0x1);   
    write(1,frame, sizeof frame);
    write(1,"12345",5);
}
