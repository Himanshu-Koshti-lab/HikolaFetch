
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>

#define RESET  "\033[0m"
#define ORANGE "\033[38;5;208m"
#define GREEN  "\033[1;32m"
#define BLUE   "\033[1;34m"

#define MAX_INFO_LINES 32
#define MAX_WIDTH 60

char *infoLines[MAX_INFO_LINES];
int infoCount = 0;

void addInfo(const char *label, const char *value) {
    char *line = malloc(256);
    snprintf(line, 256, GREEN "%-10s" RESET ": %s", label, value);
    infoLines[infoCount++] = line;
}

void fetchOS() {
    FILE *fp = fopen("/etc/os-release", "r");
    if (!fp) return;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            char *val = strchr(line, '=');
            if (val) {
                val++;
                if (*val == '"') val++; 
                val[strcspn(val, "\"\n")] = '\0';
                addInfo("OS", val);
            }
            break;
        }
    }
    fclose(fp);
}


void fetchHostname() {
    char host[256];
    gethostname(host, sizeof(host));
    addInfo("Host", host);
}

void fetchKernel() {
    struct utsname sys;
    uname(&sys);
    addInfo("Kernel", sys.release);
}

void fetchUptime() {
    struct sysinfo info;
    sysinfo(&info);
    long uptime = info.uptime;
    int hours = uptime / 3600;
    int mins = (uptime % 3600) / 60;
    char buf[64];
    snprintf(buf, sizeof(buf), "%d hours, %d mins", hours, mins);
    addInfo("Uptime", buf);
}

void fetchPackages() {
    FILE *fp = popen("dpkg -l | wc -l", "r");
    int count = 0;
    if (fp) {
        fscanf(fp, "%d", &count);
        pclose(fp);
    }
    int snap = 0;
    fp = popen("snap list | wc -l", "r");
    if (fp) {
        fscanf(fp, "%d", &snap);
        pclose(fp);
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "%d (dpkg), %d (snap)", count, snap > 0 ? snap - 1 : 0);
    addInfo("Packages", buf);
}

void fetchShell() {
    const char *val = getenv("SHELL");
    FILE *fp = popen("bash --version | head -n1", "r");
    char version[128] = "";
    if (fp && fgets(version, sizeof(version), fp)) {
        version[strcspn(version, "\n")] = '\0';
        addInfo("Shell", version);
        pclose(fp);
    } else if (val) {
        addInfo("Shell", val);
    }
}

void fetchResolution() {
    FILE *fp = popen("xdpyinfo | grep dimensions", "r");
    char line[128];
    if (fp && fgets(line, sizeof(line), fp)) {
        char *start = strstr(line, "dimensions:");
        if (start) {
            start += strlen("dimensions:");
            while (*start == ' ') start++;
            char *end = strchr(start, ' ');
            if (end) *end = '\0';
            addInfo("Resolution", start);
        }
        pclose(fp);
    }
}

void fetchDE() {
    const char *val = getenv("XDG_CURRENT_DESKTOP");
    if (val) addInfo("DE", val);
}

void fetchWM() {
    FILE *fp = popen("wmctrl -m | grep '^Name' | cut -d':' -f2", "r");
    if (fp) {
        char line[128];
        if (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = '\0';
            char *wmName = line;
            while (*wmName == ' ' || *wmName == '\t') wmName++;

            if (*wmName) {
                addInfo("WM", wmName);
            }
        }
        pclose(fp);
    }
}


void fetchGPU() {
    FILE *fp = popen("lspci | grep VGA | head -n1", "r");
    if (fp) {
        char line[256];
        if (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = '\0';
            addInfo("GPU", line);
        }
        pclose(fp);
    }
}

void fetchCPU() {
    FILE *fp = fopen("/proc/cpuinfo", "r");
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "model name", 10) == 0) {
            char *cpu = strchr(line, ':');
            if (cpu) {
                cpu++;
                while (*cpu == ' ') cpu++;
                cpu[strcspn(cpu, "\n")] = '\0';
                addInfo("CPU", cpu);
                break;
            }
        }
    }
    fclose(fp);
}

void fetchMemory() {
    struct sysinfo info;
    sysinfo(&info);
    long used = (info.totalram - info.freeram) / (1024 * 1024);
    long total = info.totalram / (1024 * 1024);
    char buf[64];
    snprintf(buf, sizeof(buf), "%ldMiB / %ldMiB", used, total);
    addInfo("Memory", buf);
}

void fetchTerminal() {
    const char *val = getenv("TERM_PROGRAM");
    if (!val) val = getenv("TERM");
    if (val) addInfo("Terminal", val);
}

void printLogoWithInfo() {
    const char *logo[] = {
        "            .-/+oossssoo+/-            ",
        "        `:+ssssssssssssssssss+:`       ",
        "      -+ssssssssssssssssssyyssss+-     ",
        "    .ossssssssssssssssssdMMMNysssso.   ",
        "   /ssssssssssshdmmNNmmyNMMMMhssssss/  ",
        "  +sssshhhyNMMNyhhyyyyhmNMMMNhssssss+  ",
        "  ossyNMMMNyMMhsssssssssshmmmhssssssso ",
        "  ossyNMMMNyMMhsssssssssshmmmhssssssso ",
        "  +sssshhhyNMMNyhhyyyyhdNMMMNhssssss+  ",
        "   /ssssssssssshdmNNNNmyNMMMMhssssss/  ",
        "    .ossssssssssssssssdMMMNysssso.     ",
        "      -+sssssssssssssssyyyssss+-       ",
        "        `:+ssssssssssssssssss+:`       ",
        "            .-/+oossssoo+/-            "
    };
    int logoLines = sizeof(logo) / sizeof(logo[0]);
    int maxLines = (logoLines > infoCount) ? logoLines : infoCount;

    for (int i = 0; i < maxLines; ++i) {
        if (i < logoLines)
            printf(ORANGE "%-40s" RESET, logo[i]);
        else
            printf("%-40s", " ");

        if (i < infoCount)
            printf("  %s", infoLines[i]);

        printf("\n");
    }
}

int main() {
    fetchOS();
    fetchHostname();
    fetchKernel();
    fetchUptime();
    fetchPackages();
    fetchShell();
    fetchResolution();
    fetchDE();
    fetchWM();
    fetchTerminal();
    fetchCPU();
    fetchGPU();
    fetchMemory();

    printLogoWithInfo();

    for (int i = 0; i < infoCount; i++) free(infoLines[i]);
    return 0;
}