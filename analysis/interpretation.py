import json
import matplotlib.pyplot as plt
import math
import time
import sys

def polygon(sides, radius=1, rotation=0, translation=None):
    one_segment = math.pi * 2 / sides

    points = [
        (math.sin(one_segment * i + rotation) * radius,
         math.cos(one_segment * i + rotation) * radius)
        for i in range(sides)]

    if translation:
        points = [[sum(pair) for pair in zip(point, translation)]
                  for point in points]

    return points

REQUEST = 0x01
REPLY   = 0x02
REQSEQ  = 0x03
REPSEQ  = 0x04
BACKOFF = 0x05
request = 0
reply = 0
reqseq = 0
repseq = 0
backoff = 0
ack = 0

speed = 1
file_name = 'Power0.json'
try:
    if sys.argv[1] != None:
        file_name = sys.argv[1]
except:
    print 'Using ' + file_name

with open(file_name) as file:
    data = json.load(file)

decoded = []
nodes = []
for j in data:
    text = ((j['_source'])['layers'])
    if int(text['frame']['frame.len']) == 5:
        # print 'ACK'
        ack = ack + 1
        decoded.append('::ACK:'+text['frame']['frame.time_relative'])
    else:
        string = (text['wpan'])['wpan.src16'] + ':' + (text['wpan'])['wpan.dst16'] + ':'
        raw_data = (text['lwm']['data']['data.data']).split(':')
        if (text['wpan'])['wpan.src16'] not in nodes:
            nodes.append((text['wpan'])['wpan.src16'])
        elif (text['wpan'])['wpan.dst16'] not in nodes:
            nodes.append((text['wpan'])['wpan.dst16'])
        if (int(raw_data[4]) == REQUEST):
            string += 'REQUEST:'
            request = request + 1
        elif (int(raw_data[4]) == REPLY):
            string += 'REPLY:'
            reply = reply + 1
        elif (int(raw_data[9]) == REQSEQ):
            string += 'REQSEQ:'
            reqseq = reqseq + 1
        elif (int(raw_data[9]) == REPSEQ):
            string += 'REPSEQ:'
            repseq = repseq + 1
        elif (int(raw_data[9]) == BACKOFF):
            string += 'BACKOFF:'
            backoff = backoff + 1
        else:
            print (raw_data)
        string += text['frame']['frame.time_relative']
        decoded.append(string)
# print (decoded)
total = request + reply + reqseq + repseq + backoff + ack
print 'Request:' + str(request)
print 'Reply:'   + str(reply)
print 'Req Seq:' + str(reqseq)
print 'Rep Seq:' + str(repseq)
print 'Backoff:' + str(backoff)
print 'Ack:'     + str(ack)
print 'Total:'   + str(total)

fig, ax = plt.subplots()

circles = []
points = polygon(len(nodes),radius = 0.5,translation=[0.5,0.5])

for idx,point in enumerate(points):
    circles.append(plt.Circle((point[0],point[1]), 0.05, fc='y'))
    ax.add_artist(circles[idx])
    plt.text(point[0],point[1], nodes[idx],
         horizontalalignment='center',
         verticalalignment='top',
         multialignment='center')
plt.axis('off')
plt.ion()

previous = -0.1
for packet in decoded:
    [src,dst,msg,time] = packet.split(':')
    if msg != 'ACK':
        j = ax.arrow(circles[nodes.index(src)].center[0], circles[nodes.index(src)].center[1], 
            circles[nodes.index(dst)].center[0]-circles[nodes.index(src)].center[0], 
            circles[nodes.index(dst)].center[1]-circles[nodes.index(src)].center[1], 
            head_width=0.05, head_length=0.1, fc='k', ec='k',length_includes_head=True)
        
        d = plt.text((circles[nodes.index(src)].center[0] + circles[nodes.index(dst)].center[0])/2, 
            (circles[nodes.index(src)].center[1] + circles[nodes.index(dst)].center[1])/2, 
            msg,
         horizontalalignment='center',
         verticalalignment='top',
         multialignment='center')
        plt.title(previous)
        plt.draw()
        plt.pause(float(time)-previous)
        previous = float(time)
        j.remove()
        d.remove()
    else:
        d = plt.text(0.5, 0.5, 
            msg,
         horizontalalignment='center',
         verticalalignment='top',
         multialignment='center')
        plt.title(previous)
        plt.draw()
        plt.pause((float(time)-previous)/speed)
        previous = float(time)
        d.remove()

        # plt.show()








