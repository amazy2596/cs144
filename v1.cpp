app <-> socketpair <-> eventloop <-> tcp_peer <-> adapter <-> TUN <-> linux kernel

eventloop use poll I/O
rule 1 : TUN readable -> adapter.read() -> tcp_peer.receive()
rule 2 : socketpair readable -> write outbound bytestream -> tcp_peer.push()
rule 3 : inbound bytestream has data && socketpair writable -> write socketpair -> app read;

---
tcp_peer
  tcp_receiver
    reassembler
      inbound bytestream
        write 
        read
  tcp_sender
    outbound bytestream 
      write 
      read

tcp_message
  sender_message
    seqno + SYN + payload + FIN + RST
  receiver_message
    ackno + windows_size + RST

Wrap32 
  wrap : 64bit seq -> 32bit seq 
  unwrap : 32bit seq -> 64bit seq

Timer
  RTO / retransmission

other modules

networkinterface
  arp cache
  etherent frame

router
  longest-prefix macthing
  TTL--
  forwarding 
---

send: 
-> app write
-> socketpair readable 
-> rule 2
-> write outbound bytestream
-> tcp_peer.push()
  -> tcp_sender.push() 
    -> sender_message
    -> tcp_peer.send()
      -> tcp_receiver.send() -> receiver_message
      -> conbine tcp_message
-> adatper.write()
  -> wrap tcp in ip 
  -> TUN write 
-> linux kernel

recv: 
-> TUN readable 
-> rule 1
-> adatper.read()
  -> TUN read
  -> unwrap tcp from ip
-> tcp_peer.receive() 
  -> sender_message -> tcp_receiver.receive()
    -> reassembler.insert()
      -> inbound bytestream
  -> receiver_message -> tcp_sender.receive()
-> inbound bytestream has data && socketpair writable 
-> rule 3
-> write socketpair
-> app read

Timer :
eventloop timeout
-> tcp_peer.tick()
  -> tcp_sender.tick()
-> RTO / retransmission 