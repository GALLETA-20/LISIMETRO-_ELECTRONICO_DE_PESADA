#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <time.h>
#include <signal.h>

#ifdef USAR_DB
#include <mysql.h>
#endif

// CONFIGURACION POR DEFECTO
#define RUTA_UART_DEFECTO "/dev/ttyS2"
#define RUTA_LOG          "/home/sallqa/LORA_UART/estacion.log"
#define DB_USUARIO        "admin"
#define DB_NOMBRE         "estacion"
#define DB_CLAVE_DEFECTO  "20A024LSALLQA1350T"

// FORMATO DEL MENSAJE DEL PIC: "%05u,%+09ld,%04ld,%06.2f,%08.1f,%02u\n"
#define LARGO_LINEA  39
#define NUM_COMAS    5
#define TAM_LINEA    128

typedef struct {
    uint32_t temp_pt100;    // CENTESIMAS DE °C
    int32_t  celda;         // VALOR CRUDO 24 BITS
    int32_t  temp_bme;      // CENTESIMAS DE °C
    double   humedad;       // %RH
    double   presion;       // Pa
    uint32_t pluvi;
} medicion_t;

// FUNCIONES
int uart_init(const char *ruta);
int SEND_COM(int fd);
int MOD_CONSOLE(char *AT_CONSOLE, size_t str_size);
int RECE_DATA(int fd);

// VARIABLES
#define LIMPIO  0
#define PROMPT  1
#define MENSAJE 2
int estado = LIMPIO;

static volatile sig_atomic_t ejecutando = 1;
static int es_interactivo = 0;

static char   linea[TAM_LINEA];
static size_t linea_n = 0;
static int    linea_desbordada = 0;


/* ===================== SISTEMA DE LOGS ===================== */

static void escribir_log(const char *nivel, const char *fmt, ...){
    FILE *f = fopen(RUTA_LOG, "a");
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);
    char fecha[32];
    strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", t);

    va_list args;
    if(f != NULL){
        fprintf(f, "[%s] [%s] ", fecha, nivel);
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fprintf(f, "\n");
        fclose(f);
    }

    if(es_interactivo){
        fprintf(stderr, "   [%s] ", nivel);
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

static void manejar_senal(int sig){
    (void)sig;
    ejecutando = 0;
}


/* ===================== BASE DE DATOS (OPCIONAL) ===================== */
#ifdef USAR_DB

static MYSQL      *db   = NULL;
static MYSQL_STMT *stmt = NULL;

static void db_cerrar(void){
    if(stmt){ mysql_stmt_close(stmt); stmt = NULL; }
    if(db)  { mysql_close(db);        db   = NULL; }
}

static int db_conectar(void){
    static const char sql[] =
        "INSERT INTO mediciones "
        "(fecha, temp_pt100, celda, temp_bme, humedad, presion, pluvi) "
        "VALUES (UTC_TIMESTAMP(), ?, ?, ?, ?, ?, ?)";

    const char *clave = getenv("LORA_DB_CLAVE");
    if(clave == NULL) clave = DB_CLAVE_DEFECTO;

    db = mysql_init(NULL);
    if(db == NULL){
        escribir_log("ERROR", "DB: mysql_init fallo");
        return -1;
    }

    if(mysql_real_connect(db, "localhost", DB_USUARIO, clave, DB_NOMBRE, 0, NULL, 0) == NULL){
        escribir_log("ERROR", "DB NO CONECTA: %s", mysql_error(db));
        db_cerrar();
        return -1;
    }

    stmt = mysql_stmt_init(db);
    if(stmt == NULL || mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0){
        escribir_log("ERROR", "DB ERROR AL PREPARAR CONSULTA: %s",
                     stmt ? mysql_stmt_error(stmt) : mysql_error(db));
        db_cerrar();
        return -1;
    }

    escribir_log("INFO", "Conexion exitosa a MariaDB (%s)", DB_NOMBRE);
    return 0;
}

static int db_insertar(const medicion_t *m){
    if(db == NULL && db_conectar() < 0) return -1;

    double   pt = m->temp_pt100 / 100.0;
    int32_t  ce = m->celda;
    double   tb = m->temp_bme / 100.0;
    double   hu = m->humedad;
    double   pr = m->presion / 100.0;
    uint32_t pl = m->pluvi;

    MYSQL_BIND b[6];
    memset(b, 0, sizeof(b));

    b[0].buffer_type = MYSQL_TYPE_DOUBLE; b[0].buffer = &pt;
    b[1].buffer_type = MYSQL_TYPE_LONG;   b[1].buffer = &ce;
    b[2].buffer_type = MYSQL_TYPE_DOUBLE; b[2].buffer = &tb;
    b[3].buffer_type = MYSQL_TYPE_DOUBLE; b[3].buffer = &hu;
    b[4].buffer_type = MYSQL_TYPE_DOUBLE; b[4].buffer = &pr;
    b[5].buffer_type = MYSQL_TYPE_LONG;   b[5].buffer = &pl; b[5].is_unsigned = 1;

    if(mysql_stmt_bind_param(stmt, b) != 0 || mysql_stmt_execute(stmt) != 0){
        escribir_log("ERROR", "DB ERROR AL INSERTAR: %s", mysql_stmt_error(stmt));
        db_cerrar();
        return -1;
    }
    return 0;
}
#endif


/* ===================== ANALISIS DE LA LINEA ===================== */

static int contar_comas(const char *l){
    int n = 0;
    for(; *l; l++) if(*l == ',') n++;
    return n;
}

static const char *validar(const medicion_t *m){
    if(m->temp_bme < -4000 || m->temp_bme > 8500)       return "temperatura BME fuera de rango";
    if(m->humedad  < 0.0   || m->humedad  > 100.0)      return "humedad fuera de rango";
    if(m->presion  < 30000.0 || m->presion > 110000.0)  return "presion fuera de rango";
    if(m->pluvi > 99)                                   return "pluviometro fuera de rango";
    return NULL;
}

static int parsear_linea(const char *l, medicion_t *m, const char **error){
    unsigned a, f;
    long     b;
    int      c;
    double   d, e;
    int      pos = 0;

    if(strlen(l) != LARGO_LINEA){
        *error = "largo distinto de 39 caracteres";
        return -1;
    }

    if(sscanf(l, "%u,%ld,%d,%lf,%lf,%u%n", &a, &b, &c, &d, &e, &f, &pos) != 6 || l[pos] != '\0'){
        *error = "formato incorrecto";
        return -1;
    }

    m->temp_pt100 = (uint32_t)a;
    m->celda      = (int32_t)b;
    m->temp_bme   = (int32_t)c;
    m->humedad    = d;
    m->presion    = e;
    m->pluvi      = (uint32_t)f;

    *error = validar(m);
    return (*error == NULL) ? 0 : -1;
}

static void procesar_linea(const char *l){
    medicion_t  m;
    const char *error = NULL;

    if(contar_comas(l) != NUM_COMAS) return;

    if(parsear_linea(l, &m, &error) < 0){
        escribir_log("WARN", "DATO RECHAZADO (%s): '%s'", error, l);
        return;
    }

    if(es_interactivo){
        printf("   [DATO] pt100=%.2f C | celda=%ld | bme=%.2f C | hum=%.2f %% | pres=%.1f hPa | pluvi=%u\n",
               m.temp_pt100 / 100.0, (long)m.celda, m.temp_bme / 100.0,
               m.humedad, m.presion / 100.0, (unsigned)m.pluvi);
    }

#ifdef USAR_DB
    if(db_insertar(&m) == 0 && es_interactivo){
        printf("   [GUARDADO EN MARIADB]\n");
    }
#endif
}

static void acumular(char c){
    if(c == '\n'){
        if(!linea_desbordada){
            linea[linea_n] = '\0';
            procesar_linea(linea);
        } else {
            escribir_log("WARN", "Linea desbordada (supero %d bytes)", TAM_LINEA);
        }
        linea_n = 0;
        linea_desbordada = 0;
        return;
    }

    if(linea_n < TAM_LINEA - 1) linea[linea_n++] = c;
    else                        linea_desbordada = 1;
}


/* ===================== PROGRAMA PRINCIPAL ===================== */

int main(int argc, char *argv[]){
    es_interactivo = isatty(STDIN_FILENO);

    signal(SIGTERM, manejar_senal);
    signal(SIGINT,  manejar_senal);

    // MODO PRUEBA: ./main -t "LINEA"
    if(argc > 2 && strcmp(argv[1], "-t") == 0){
        procesar_linea(argv[2]);
#ifdef USAR_DB
        db_cerrar();
#endif
        return 0;
    }

    const char *ruta = RUTA_UART_DEFECTO;
    if(argc > 1) ruta = argv[1];

    escribir_log("INFO", "=== INICIANDO SERVICIO ESTACION LORA (UART: %s) ===", ruta);

    int fd = uart_init(ruta);
    if(fd < 0){
        escribir_log("ERROR", "No se pudo abrir/configurar el puerto %s", ruta);
        return 1;
    }

    if(es_interactivo) printf("CONSOLA DE COMANDO AT\n");

    struct pollfd fds[2];
    nfds_t nfds = 0;

    if(es_interactivo){
        fds[0].fd = STDIN_FILENO; fds[0].events = POLLIN;
        fds[1].fd = fd;           fds[1].events = POLLIN;
        nfds = 2;
    } else {
        fds[0].fd = fd;           fds[0].events = POLLIN;
        nfds = 1;
    }

    while(ejecutando){
        if(es_interactivo && estado == LIMPIO){
            printf("[USUARIO] > ");
            fflush(stdout);
            estado = PROMPT;
        }

        int r = poll(fds, nfds, 1000); // Timeout 1s para revisar señal de apagado
        if(r < 0){
            if(!ejecutando) break;
            escribir_log("ERROR", "Fallo en poll()");
            break;
        }
        if(r == 0) continue;

        int idx_uart = es_interactivo ? 1 : 0;

        if(fds[idx_uart].revents & POLLIN){
            if(RECE_DATA(fd) < 0) break;
        }

        if(fds[idx_uart].revents & (POLLERR | POLLHUP | POLLNVAL)){
            escribir_log("ERROR", "Dispositivo UART desconectado o con error (%s)", ruta);
            break;
        }

        if(es_interactivo && (fds[0].revents & POLLIN)){
            int v = SEND_COM(fd);
            if(v < 0) break;
            if(v == 1) continue;
        }
    }

    close(fd);
#ifdef USAR_DB
    db_cerrar();
#endif

    escribir_log("INFO", "=== SERVICIO ESTACION LORA DETENIDO ===");
    if(es_interactivo) printf("\n");
    return 0;
}


int uart_init(const char *ruta){
    int fd = open(ruta, O_RDWR | O_NOCTTY);
    if(fd < 0) return -1;

    struct termios tty;
    if((tcgetattr(fd, &tty)) < 0){
        close(fd);
        return -1;
    }

    cfmakeraw(&tty);
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | CSTOPB);

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;

    if((tcsetattr(fd, TCSANOW, &tty)) < 0){
        close(fd);
        return -1;
    }

    return fd;
}


int SEND_COM(int fd){
    char AT_CONSOLE[100];
    estado = LIMPIO;

    if((fgets(AT_CONSOLE, sizeof(AT_CONSOLE), stdin)) == NULL) return -1;
    if(AT_CONSOLE[0] == '\n') return 1;
    if(MOD_CONSOLE(AT_CONSOLE, sizeof(AT_CONSOLE)) < 0) return -1;

    int tam = (int)strlen(AT_CONSOLE);
    if((write(fd, AT_CONSOLE, tam)) < tam){
        escribir_log("ERROR", "Fallo al escribir comando AT en UART");
        return -1;
    }

    tcdrain(fd);
    return 0;
}


int MOD_CONSOLE(char *AT_CONSOLE, size_t str_size){
    size_t texto = strcspn(AT_CONSOLE, "\n");
    if(str_size <= (texto + 2)) return -1;

    AT_CONSOLE[texto] = '\r';
    AT_CONSOLE[texto + 1] = '\n';
    AT_CONSOLE[texto + 2] = '\0';
    return 0;
}


int RECE_DATA(int fd){
    char buff[256];
    ssize_t n = read(fd, buff, sizeof(buff));

    if(n > 0){
        for(ssize_t i = 0; i < n; i++){
            if(buff[i] == '\r') continue;

            if(es_interactivo){
                if(estado == PROMPT){
                    putchar('\n');
                    estado = LIMPIO;
                }
                if(estado == LIMPIO){
                    printf("[LORA 1] > ");
                    estado = MENSAJE;
                }
                putchar(buff[i]);
                if(buff[i] == '\n') estado = LIMPIO;
            }

            acumular(buff[i]);
        }
        if(es_interactivo) fflush(stdout);
    } else if(n < 0){
        escribir_log("ERROR", "Fallo al leer datos del UART");
        return -1;
    }
    return 0;
}