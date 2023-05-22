from bs4 import BeautifulSoup
import bs4
import os


src_path = 'out/zsbgs'
dir_path = 'txts/zsbgs'

def tag_visible(e):
    if e.parent.name in ['style', 'script', 'head', 'title', 'meta', '[document]']:
        return False
    if isinstance(e, bs4.element.Comment) or e.name == "img":
        return False
    return True

def totext(soup):
    texts = soup.findAll(string=True)
    visible_texts = filter(tag_visible, texts)  
    return u"".join(t.strip() for t in visible_texts)

i=1

for file in os.listdir(src_path):
    with open(os.path.join(src_path, file), 'r') as f1:
        soup = BeautifulSoup(f1,features="lxml")
        i=i+1
        print(f'{file}\n{i}')
        anchor = soup.find("a",string="关闭窗口") or soup.find("p",string="【本文属内部通告，未经发文单位同意，不得截屏、拍图或复制转发】")
        subtree = anchor.parent.parent.parent(recursive=False)[1](recursive=False)[0](recursive=False)[0]
        title = totext(subtree(recursive=False)[0])
        content = totext(subtree(recursive=False)[2])
        # print(title)
        # print(content)
        with open(os.path.join(dir_path, file.split('.')[0]),'w') as f2:
            f2.write(f'{title} \n')
            f2.write(content)
            