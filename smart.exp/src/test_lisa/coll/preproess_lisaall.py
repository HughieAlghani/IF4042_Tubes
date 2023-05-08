with open('lisa.all', 'r') as f:
    content = f.readlines()

new_content = []
flag_new_doc = False
added_w_flag = False

i = 1
for line in content:
    if line.startswith('Document'):
        print("Document ke - ", i)
        i += 1
        flag_new_doc = True
        added_w_flag = False
        new_content.append(line)
        new_content.append('~T\n')
    elif line.strip() == '':
        print("Tambahkan ~W di document ke - ", i)
        if flag_new_doc and not added_w_flag:
            new_content.append('~W\n')
            added_w_flag = True
            flag_new_doc = False
    else:
        new_content.append(line)

    if line.startswith('********************************************'):
        print("Document ke - ", i)
        print("added w flag = ", added_w_flag)

if new_content[-1] == '\n':
    new_content.pop()

with open('lisa_modified.all', 'w') as f:
    f.writelines(new_content)