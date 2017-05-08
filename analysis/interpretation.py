import json
REQUEST = 0x01
REPLY   = 0x02
REQSEQ  = 0x03
REPSEQ  = 0x04
BACKOFF = 0x05

with open('/Users/lakshbhatia/Downloads/Power0.json') as file:
    data = json.load(file)
request = 0
reply = 0
reqseq = 0
repseq = 0
backoff = 0
ack = 0

decoded = []
for j in data:
    text = ((j['_source'])['layers'])
    if int(text['frame']['frame.len']) == 5:
        print 'ACK'
        ack = ack + 1
        decoded.append('::ACK')
    else:
        string = (text['wpan'])['wpan.src16'] + ':' + (text['wpan'])['wpan.dst16'] + ':'
        raw_data = (text['lwm']['data']['data.data']).split(':')
        if (int(raw_data[4]) == REQUEST):
            string += 'REQUEST'
            request = request + 1
        elif (int(raw_data[4]) == REPLY):
            string += 'REPLY'
            reply = reply + 1
        elif (int(raw_data[9]) == REQSEQ):
            string += 'REQSEQ'
            reqseq = reqseq + 1
        elif (int(raw_data[9]) == REPSEQ):
            string += 'REPSEQ'
            repseq = repseq + 1
        elif (int(raw_data[9]) == BACKOFF):
            string += 'BACKOFF'
            backoff = backoff + 1
        else:
            print (raw_data)
        decoded.append(string)
print len(decoded)
print 'Request:' + str(request)
print 'Reply:'   + str(reply)
print 'Req Seq:' + str(reqseq)
print 'Rep Seq:' + str(repseq)
print 'Backoff:' + str(backoff)
print 'Ack:'     + str(ack)