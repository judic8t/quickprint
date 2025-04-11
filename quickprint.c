#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 8192
#define IPP_PORT 631  // Standard IPP port

void print_usage(char *prog_name) {
    fprintf(stderr, "Usage: %s <file> <printer_ip> <color|bw> <onesided|twosided> <portrait|landscape>\n", prog_name);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        print_usage(argv[0]);
    }

    const char *filename = argv[1];
    const char *printer_ip = argv[2];
    const char *color_mode = argv[3];
    const char *sides = argv[4];
    const char *orientation = argv[5];

    // Validate arguments
    if (strcmp(color_mode, "color") != 0 && strcmp(color_mode, "bw") != 0) {
        fprintf(stderr, "Color mode must be 'color' or 'bw'\n");
        print_usage(argv[0]);
    }
    if (strcmp(sides, "onesided") != 0 && strcmp(sides, "twosided") != 0) {
        fprintf(stderr, "Sides must be 'onesided' or 'twosided'\n");
        print_usage(argv[0]);
    }
    if (strcmp(orientation, "portrait") != 0 && strcmp(orientation, "landscape") != 0) {
        fprintf(stderr, "Orientation must be 'portrait' or 'landscape'\n");
        print_usage(argv[0]);
    }

    // Read file content
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *file_content = malloc(file_size);
    fread(file_content, 1, file_size, file);
    fclose(file);

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        free(file_content);
        exit(1);
    }

    // Connect to printer
    struct sockaddr_in printer_addr;
    printer_addr.sin_family = AF_INET;
    printer_addr.sin_port = htons(IPP_PORT);
    if (inet_pton(AF_INET, printer_ip, &printer_addr.sin_addr) <= 0) {
        perror("Invalid printer IP");
        close(sock);
        free(file_content);
        exit(1);
    }

    if (connect(sock, (struct sockaddr*)&printer_addr, sizeof(printer_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        free(file_content);
        exit(1);
    }

    // IPP Header and Attributes (binary format)
    char ipp_request[BUFFER_SIZE];
    int offset = 0;

    // IPP version (2.0)
    ipp_request[offset++] = 0x01;  // Major version
    ipp_request[offset++] = 0x01;  // Minor version
    
    // Operation: Print-Job (0x0002)
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = 0x02;
    
    // Request ID
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = 0x01;

    // Operation attributes tag
    ipp_request[offset++] = 0x01;  // Start operation-attributes
    
    // Charset
    ipp_request[offset++] = 0x47;  // charset type
    ipp_request[offset++] = 0x00;  // charset type
    ipp_request[offset++] = 0x12;  // charset type
    char achar[] = {"attributes-charset"};
    memcpy(ipp_request + offset, achar, sizeof(achar) - 1);
    offset += sizeof(achar) - 1;
    ipp_request[offset++] = 0x00;  // charset type
    ipp_request[offset++] = 0x05;  // charset type
    char utfchar[] = {"utf-8"};
    memcpy(ipp_request + offset, utfchar, sizeof(utfchar) - 1);
    offset += sizeof(utfchar) - 1;

    // Natural language
    ipp_request[offset++] = 0x48;  // natural-language type
    ipp_request[offset++] = 0x00;  // natural-language type
    ipp_request[offset++] = 0x1b;  // natural-language type
    char anlchar[] = {"attributes-natural-language"};
    memcpy(ipp_request + offset, anlchar, sizeof(anlchar));
    offset += sizeof(anlchar) - 1;
    ipp_request[offset++] = 0x00;  // natural-language type
    ipp_request[offset++] = 0x05;  // natural-language type
    char enuschar[] = {"en-us"};
    memcpy(ipp_request + offset, enuschar, sizeof(enuschar) - 1);
    offset += sizeof(enuschar) - 1;

    // Printer URI
    ipp_request[offset++] = 0x45;  // uri type
    char printer_uri[256];
    snprintf(printer_uri, sizeof(printer_uri), "ipp://%s/ipp/print", printer_ip);
    int uri_len = strlen(printer_uri);
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = 0x0b;  // "printer-uri" length
    memcpy(ipp_request + offset, "printer-uri", 11);
    offset += 11;
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = uri_len;
    memcpy(ipp_request + offset, printer_uri, uri_len);
    offset += uri_len;

    // Job name
    ipp_request[offset++] = 0x42;
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = 0x08;
    char job_name_label[] = {"job-name"};
    memcpy(ipp_request + offset, job_name_label, sizeof(job_name_label) - 1);
    offset += sizeof(job_name_label) - 1;
    int file_name_size = strlen(filename);
    ipp_request[offset++] = (file_name_size >> 8) & 0xFF;
    ipp_request[offset++] = file_name_size & 0xFF;
    memcpy(ipp_request + offset, filename, file_name_size);
    offset += file_name_size;

    // Job name
    ipp_request[offset++] = 0x42;
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = 0x14;
    char user_name_label[] = {"requesting-user-name"};
    memcpy(ipp_request + offset, user_name_label, sizeof(user_name_label) - 1);
    offset += sizeof(user_name_label) - 1;
    char user_name[] = {"quickprint"};
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = 0x0A;
    memcpy(ipp_request + offset, user_name, sizeof(user_name) - 1);
    offset += sizeof(user_name) - 1;

    // Job attributes tag
    ipp_request[offset++] = 0x02;  // Start job-attributes

    // Color mode
    ipp_request[offset++] = 0x44;  // keyword type
    const char *ipp_color_label = "print-color-mode";
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = strlen(ipp_color_label);
    memcpy(ipp_request + offset, ipp_color_label, strlen(ipp_color_label));
    offset += strlen(ipp_color_label);
    const char *ipp_color = strcmp(color_mode, "color") == 0 ? "color" : "monochrome";
    int color_len = strlen(ipp_color);
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = color_len;
    memcpy(ipp_request + offset, ipp_color, color_len);
    offset += color_len;

    // Sides
    ipp_request[offset++] = 0x44;  // keyword type
    const char *ipp_sides_label = "sides";
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = strlen(ipp_sides_label);
    memcpy(ipp_request + offset, ipp_sides_label, strlen(ipp_sides_label));
    offset += strlen(ipp_sides_label);
    const char *ipp_sides = strcmp(sides, "onesided") == 0 ? "one-sided" : "two-sided-long-edge";
    int sides_len = strlen(ipp_sides);
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = sides_len;
    memcpy(ipp_request + offset, ipp_sides, sides_len);
    offset += sides_len;

    // Orientation
    ipp_request[offset++] = 0x21;  // integer type
    const char* ipp_orientation_label = "orientation-requested";
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = strlen(ipp_orientation_label);
    memcpy(ipp_request + offset, ipp_orientation_label, strlen(ipp_orientation_label));
    offset += strlen(ipp_orientation_label);
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = 0x04;  // 4 bytes
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = 0x00;
    ipp_request[offset++] = strcmp(orientation, "portrait") == 0 ? 0x03 : 0x04;

    // End of attributes
    ipp_request[offset++] = 0x03;  // End of attributes

    // HTTP Request
    char http_request[BUFFER_SIZE];
    int content_length = offset + file_size;
    snprintf(http_request, BUFFER_SIZE,
        "POST /ipp/print HTTP/1.1\r\n"
        "Content-Type: application/ipp\r\n"
        "Host: %s\r\n"
        "Content-Length: %d\r\n"
        "\r\n",
        printer_ip, content_length
    );

    // Send HTTP header
    send(sock, http_request, strlen(http_request), 0);
    // Send IPP request
    send(sock, ipp_request, offset, 0);
    // Send file content
    send(sock, file_content, file_size, 0);

    // Receive response
    char response[BUFFER_SIZE];
    int bytes_received = recv(sock, response, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        printf("Printer response:\n%s\n", response);
    }

    // Cleanup
    free(file_content);
    close(sock);
    return 0;
}
