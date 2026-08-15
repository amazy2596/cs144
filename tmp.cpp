app <-> socketpair <-> eventloop <-> tcp_peer <-> Adapter <-> TUN <-> linux kernal

eventloop use poll I/O:
rule 1 : TUN readable -> Adapter.read() -> tcp_peer.receive()
rule 2 : socketpair readable -> write outbound bytestream -> tcp_peer.push()
rule 3 : inbound bytestream has data && socket writable -> write socketpair -> app read

---
tcp_peer
  tcp_receiver
    reassembler
      inbound bytestream
        writer
        reader

  tcp_sender
    outbound bytestream
      writer
      reader

tcp_message
  receiver_message
    ACK + windows_size + RST
  sender_message
    seq + SYN + payload + FIN + RST

wrap32
  wrap : 64bit seq -> 32bit seq
  unwrap : 32bit seq -> 64bit seq

Timer
  RTO / retransmission

other modules 
  networkinterface
    arp cache 
    ethernet frame
    pending datagrams
  router
    longest-prefix matching
    TTL decrement
    forwarding 
---

send: 
-> app write 
-> socketpair readable 
-> rule 2 
-> write outbound bytestream 
-> tcp_peer.push() 
-> adapter.write()
-> wrap tcp in ip 
-> TUN write 
-> linux kernel

recv: 
-> TUN readable 
-> rule 1 
-> adapter.read() 
-> unwrap tcp from ip 
-> tcp_peer.receive()
  -> tcp_receiver.receive()
    -> reassembler.insert() 
      -> inbound bytestream
  -> tcp_sender.receive()
-> socketpair writable 
-> rule 3 
-> read inbound bytestream
-> socketpair.write()
-> app read

timer:
eventloop timeout
-> tcp_peer.tick()
-> tcp_sender.tick()
-> RTO / retransmission