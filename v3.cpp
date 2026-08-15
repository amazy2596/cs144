app <-> socketpair <-> eventloop <-> tcp_peer <-> adatper <-> TUN <-> linux kernel

eventloop poll 
rule 1 : TUN readable -> adatper.read() -> tcp_peer.receive()
rule 2 : app write -> socketpair readable -> write outbound bytestream -> tcp_peer.push()
rule 3 : inbound bytestream has data && socketpair writable -> write socketpair -> app read

--- 
tcp_peer
  tcp_sender
    outbound bytestream
  tcp_receiver
    reassmebler
      inbound bytestream

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
-> adapter.write()
  -> wrap tcp in ip
  -> TUN write
-> linux kernel 

recv: 
-> TUN readable
-> rule 1
-> adapter.read()
  -> TUN read
  -> unwrap tcp from ip
-> tcp_peer.receive()
  -> sender_message -> tcp_receiver.receive()
    -> reassembler.insert()
      -> inbound bytestream
  -> receiver_message -> tcp_sender.receive()
-> inbound bytestream has data && socketpair writable 
-> rule 3 
-> wrtie socketpair 
-> app read 