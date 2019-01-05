package main

import (
	"fmt"
	"strconv"

	"github.com/miekg/dns"
)

const ns_domain = "nstunnel.example.net."

func handleTunnel(w dns.ResponseWriter, r *dns.Msg) {
	m := new(dns.Msg)
	m.SetReply(r)
	m.Authoritative = true

	fmt.Println("Got", len(r.Question), "questions")
	for _, q := range r.Question {
		fmt.Println("  ", q.Name)
		labels := dns.SplitDomainName(q.Name)
		v, _ := strconv.Atoi(labels[0])
		rr := &dns.TXT{
			Hdr: dns.RR_Header{Name: q.Name, Rrtype: dns.TypeTXT, Class: dns.ClassINET, Ttl: 0},
			Txt: []string{strconv.Itoa(v + 1)},
		}
		m.Answer = append(m.Answer, rr)
	}

	w.WriteMsg(m)
}

func main() {
	dns.HandleFunc(ns_domain, handleTunnel)
	server := &dns.Server{Addr: ":53", Net: "udp", TsigSecret: nil, ReusePort: true}
	e := server.ListenAndServe()
	if e != nil {
		fmt.Println("ListenAndServe error", e)
	}
}
