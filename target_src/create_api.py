#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys
import os
from pathlib import Path
import datetime

def read_prog(fileName):
    res = {}
    res['name'] = fileName
    res['size'] = os.path.getsize(fileName)
    with open(fileName, mode='rb') as file:
        fileContent = file.read()
        code = ''
        for i in range(len(fileContent)):
            code = code + hex(fileContent[i]) + ', '
        res['data'] = code
    return res

def create_header_file(progs):
    f = open('target_src/target_progs.h', 'w')
    f.write('// automatically created ' + datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S') + '\n')
    f.write('\n')
    f.write('#ifndef TARGET_SRC_TARGET_PROGS_H_\n')
    f.write('#define TARGET_SRC_TARGET_PROGS_H_\n')
    f.write('\n')
    f.write('#include <stdint.h>\n')
    f.write('\n')
    f.write('typedef enum {\n')
    for k in progs.keys():
        f.write('    ' + k + ',\n')
    f.write('}progs_typ;\n');
    f.write('\n')
    f.write('uint32_t target_progs_get_size(progs_typ prog);\n')
    f.write('uint8_t* target_progs_get_code(progs_typ prog);\n')
    f.write('\n')
    f.write('#endif /* TARGET_SRC_TARGET_PROGS_H_ */\n')
    f.write('\n')
    f.close()

def create_c_file(progs):
    f = open('target_src/target_progs.c', 'w')
    f.write('// automatically created ' + datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S') + '\n')
    f.write('#include <stddef.h>\n')
    f.write('#include "target_progs.h"\n')
    f.write('\n')
    for k in progs.keys():
        f.write('static uint8_t ' + k + '_code[] = {\n')
        f.write(str(progs[k]['data']) + '\n')
        f.write('};\n')
        f.write('\n')
    f.write('uint32_t target_progs_get_size(progs_typ prog)\n')
    f.write('{\n')
    f.write('    switch(prog)\n')
    f.write('    {\n')
    for k in progs.keys():
        f.write('    case ' + k + ': return ' + str(progs[k]['size']) + ';\n')
    f.write('    default: return 0;\n')
    f.write('    }\n')
    f.write('}\n')
    f.write('\n')
    f.write('uint8_t* target_progs_get_code(progs_typ prog)\n')
    f.write('{\n')
    f.write('    switch(prog)\n')
    f.write('    {\n')
    for k in progs.keys():
        f.write('    case ' + k + ': return &' + k + '_code[0];\n')
    f.write('    default: return NULL;\n')
    f.write('    }\n')
    f.write('}\n')
    f.write('\n')
    f.close()


if __name__ == '__main__':
    #find all progs
    progs = {}
    for f in sys.argv[1:]:
        print('now working on ' + f)
        base = Path(f).stem
        progs[base.upper()] = read_prog(f)
    # create files
    create_header_file(progs)
    create_c_file(progs)


