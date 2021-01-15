##
# @author huatuo

# -*- coding:utf-8 -*-
import sys
import os


def conf2h(conf, header):
    header_f = open(header, 'w')
    conf_f = open(conf, 'r')
    conf_lines = conf_f.readlines()
    for l in conf_lines:
        if l.startswith("CONFIG_"):
            ori_key = l.split('=', 1)[0]
            ori_value = l.split('=', 1)[1]

            def_head = "#define "
            def_key = ori_key.replace("CONFIG_", '', 1) + ' '
            def_value = ori_value if ori_value[0] != 'y' else ori_value.replace('y', '1', 1)

            header_f.write(def_head + def_key + def_value)
        elif l.startswith("#"):
            header_f.write(l.replace('#', "//", 1))
        else:
            header_f.write(l)
    header_f.close()
    conf_f.close()


if __name__ == "__main__":
    conf_file=sys.argv[1]
    h_file=sys.argv[2]
    if os.path.exists(conf_file):
        conf2h(conf_file, h_file)
    else:
        print("can't find file: ", conf_file)
        exit(1)
