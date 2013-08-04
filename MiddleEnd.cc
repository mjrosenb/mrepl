#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "Line.h"
#include <elf.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
// hardcode us to 64 bit elf for now, this is easy enough
// to make conditional later
#define Elf_Shdr Elf64_Shdr
#define Elf_Ehdr Elf64_Ehdr
#include "BackEnd.h"
#include "Debug.h"

// fork and exec a program, with arguments.
void run(const char *file, char *const argv[], RunLog &log)
{
    // for capturing stdout
    int pipeout[2];
    // for capturing stderr.
    int pipeerr[2];
    pipe(pipeout);
    pipe(pipeerr);
    int readfds[2];
    readfds[0] = pipeout[0];
    readfds[1] = pipeerr[0];
    pid_t pid = fork();
    if (pid < 0)
        perror("Invoking as");
    if (pid != 0) {
        int status;
        bool finished = false;
        fd_set rfds;
        char **(logptr[2]) = {&log.out, &log.error};
        int bufsize[2] = {4096, 4096};
        int writesize[2] = {0,0};

        for (int i = 0; i < 2; i++) {
            *logptr[i] = (char*)malloc(bufsize[i]);
        }

        int maxFD = pipeout[0] > pipeerr[0] ? pipeout[0] : pipeerr[0];
        int opencount = 2;
        do {
            FD_ZERO(&rfds);
            for (int i = 0; i < 2; i++) {
                if (readfds[i] > 0)
                    FD_SET(readfds[i], &rfds);
            }
            int fdact = select(maxFD+1, &rfds, NULL, NULL, NULL);
            for (int i = 0; i < 2; i++) {
                if (FD_ISSET(readfds[i], &rfds)) {
                    char buf[4096];
                    int size = read(readfds[i], buf, 4096);
                    if (size < 0 && errno == EINVAL) {
                        // close off the fd on this end as well
                        close(readfds[i]);
                        readfds[i] = -1;
                        opencount--;
                    }
                    if (writesize[i]+size > bufsize[i]) {
                        bufsize[i] *= 2;
                        *logptr[i] = (char*)realloc(*logptr[i], bufsize[i]);
                    }
                    memcpy(*logptr[i] + writesize[i], buf, size);
                    writesize[i] += size;
                }
            }
        } while (opencount);
        log.outsize = bufsize[0];
        log.errsize = bufsize[1];
        if (waitpid(pid, &status, 0) && WIFEXITED(status)) {
            log.exitStatus = WEXITSTATUS(status);
        }
    } else {
        // close the read ends of these pipes, just because.
        close(pipeout[0]);
        close(pipeout[1]);

        // duplicate the pipes down to the standard locations.
        dup2(pipeout[1], STDOUT_FILENO);
        dup2(pipeerr[1], STDERR_FILENO);

        // run the target program.
        execvp(file, argv);
    }
}

// Given the name of a file, mmap it, find out the location of the start of every
// instruction, so we can reliably report the current instruction (Dissassembly is for chumps)
void extractLineInfo(const char *exefile, Snippet *snip, ExecutableInfo &info)
{
    fprintf(stderr, "Using file: %s\n", exefile);
    // Open then mmap the file
    int fd = open(exefile, O_RDONLY);
    if (fd < 0) {
        perror("opening executable");
        exit(1);
    }

    // Grab the size so we know how much to mmap
    struct stat stats;
    int ret = fstat(fd, &stats);
    if (ret == -1) {
        perror("stating executable");
        exit(1);
    }

    // mmap the file, and grab where it is placed as an array of chars
    char *file = reinterpret_cast<char*>(mmap(NULL, stats.st_size, PROT_READ, MAP_SHARED,  fd, 0));
    if (file == (void*)-1) {
        perror("mmaping executable");
        exit(1);
    }
    // View the start of the file as an ELF header
    Elf_Ehdr *header = reinterpret_cast<Elf64_Ehdr*>(file);

    // do some very basic sanity checking
    assert(*(int*)header->e_ident == *(int*)ELFMAG);
    assert(header->e_ident[EI_VERSION] == EV_CURRENT);

    Elf_Shdr *sections = reinterpret_cast<Elf64_Shdr*>(file + header->e_shoff);

    // these assertions may not hold, if there are too many sections, but I never
    // generate that many sections, and honestly, how many sections can you type
    // in in the repl?
    assert(header->e_shstrndx != SHN_UNDEF);
    assert(header->e_shstrndx != SHN_XINDEX);
    // there is a section that contains nothing but strings,
    // every other string-like thing in the file is just
    // an index into the data of this section
    Elf_Shdr *stringSection = &sections[header->e_shstrndx];
    fprintf(stderr,"String table's header is %d\n",  header->e_shstrndx);
    // get the actual table
    char *stringTable = file + stringSection->sh_offset;

    // loop until we find the section labeled "lineMap"
    for (int idx = 0; idx < header->e_shnum; idx++) {
        char *name = stringTable + sections[idx].sh_name;
        fprintf(stderr, "found section: '%s'\n", name);
        if (strcmp(name, "lineMap") == 0) {
            log("Found It\n");
            log("offset is: %d\n", sections[idx].sh_offset);
            void **map = reinterpret_cast<void**>(file + sections[idx].sh_offset);
            snip->assignInsts(map);
            log("found sentinel: %p\n", *map);
            info.sentinel = *map;
            break;
        }
    }
    munmap(file, stats.st_size);
    close(fd);
}

static const char *template_ = "/tmp/mreplXXXXXX.s";
static const int LEN = strlen(template_)+1;
// Like the name says, generate machine code given a program snippet, s.
void generateMachineCode(Snippet *s, ExecutableInfo &info)
{
    // buffer that will hold the name of the assembly file
    char asname[LEN];
    // buffer that will hold the name of the object file
    char objname[LEN];
    // buffer that will hold the name of the executable-- dynamically allocated because
    // this needs to survive over to other phases, namely, the execute phase
    char *exename = new char[LEN];
    strncpy(asname, template_, LEN);
    // make the actual file, and open it
    int fd = mkstemps(asname, 2);

    // copy the filenames' bits so we can stomp over them
    strncpy(objname, asname, LEN);
    strncpy(exename, asname, LEN);
    // len - 2 is currently 's', replace with 'o' to transform foo.s -> foo.o
    objname[LEN-2] = 'o';
    // One before the suffix is the '.', replace with a terminator to
    // transform  foo.s -> foo
    exename[LEN-3] = '\0';

    // Take the file descriptor we got before, and change it into a FILE stream
    // for ease of formatting
    FILE *f = fdopen(fd, "w");
    // fixed x86 header
    fprintf(f, ".globl _start\n");
    fprintf(f, "_start:\n");
    fprintf(f, "int3\n");
    // Dump the instruction stream into the file, leaving lots of labels in the way
    s->dump(f);

    // Now, in order to map the addresses back to instructions, we want to
    // reference the label that was placed at the beginning of every instruction
    // in the previous phase, this will create a contiguous table of addresses
    // in the exact same order that the instructions were emitted in

    // generate one last label as a way to identify when execution reaches the
    // end of written code
    fprintf(f, "last:\n");

    // Put this table in its own section, and give it a label
    fprintf(f, ".section lineMap\n");
    fprintf(f, "lineMAP_:\n");
    // actually write out all of the refrences
    s->dumpTable(f);
    // refrence the last label as well
    fprintf(f, ".quad last\n");
    fclose(f);
    // run the assembler, this kills the current program if it fails!
    // type carefully!
    char *const asargs[] = {"as", asname, "-o", objname, NULL};
    run("as", asargs, info.aslog);
    // run the linker, once again failure leads to termination
    char *const ldargs[] = {"ld", objname, "-o", exename, NULL};
    run("ld", ldargs, info.ldlog);

    // at this point, in exename, there  should be an executable binary!
    // we want to parse the binary, extract some information that we've embedded
    // in it, and finally kick off to the backend for execution
    extractLineInfo(exename, s, info);

    // store a bunch of information for the next phase
    info.exename = exename;
    info.topLevel = s;
}
