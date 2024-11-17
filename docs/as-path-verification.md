## AS_PATH Verification
[I-D.ietf-idr-deprecate-as-set-confed-set] specifies that "treat-as-
withdraw" error handling [RFC7606] SHOULD be applied to routes with
AS_SET in the AS_PATH.  In the current document, routes with AS_SET
are given Invalid evaluation in the AS_PATH verification procedures
(Section 7.2 and Section 7.3).  See Section 7.4 for how routes with
Invalid AS_PATH are handled.

7.1.  Principles

Let the sequence {AS(N), AS(N-1),..., AS(2), AS(1)} represent the
AS_PATH in terms of unique ASNs, where AS(1) is the origin AS and
AS(N) is the most recently added AS and neighbor of the receiving/
verifying AS.  N is the length of the received AS_PATH in unique
ASes.  Let AS(N+1) represent the receiving/verifying AS.

                   AS(L) ............. AS(K)
                    /                     \
                AS(L+1)                  AS(K-1)
                   .                       .
                  .                         .
   (down-ramp)   .                           .  (up-ramp)
                .                             .
               .                               .
             AS(N-1)                          AS(2)
               /                                \
            AS(N)                               AS(1)
             /                                (Origin AS)
   Receiving & verifying AS (AS(N+1))
          (Customer)

       Each ramp has consecutive customer-to-provider hops in the bottom-to-top direction

        Figure 2: Illustration of up-ramp and down-ramp.

The AS_PATH may in general have both an up-ramp (on the right
starting at AS(1)) and a down-ramp (on the left starting at AS(N)).
The up-ramp starts at AS(1) and each hop AS(i) to AS(i+1) represents
Customer and Provider peering relationship.  The down-ramp runs
backward from AS(N) to AS(L).  In the down-ramp, each pair AS(j) to
AS(j-1) represents Customer and Provider peering relationship.  If
there are no hops or just one hop between the apexes of the up-ramp
and the down-ramp, then the AS_PATH is valid (valley free).

If the sum of lengths of up-ramp and down-ramp is less than N, it is
invalid: the prefix was leaked or AS_PATH was malformed.

Sure! Let’s go through this step by step in simpler terms and make 
it more explanatory.

### What’s Happening?

We’re looking at how internet traffic flows between different **Autonomous 
Systems (ASes)**, which are networks run by organizations like ISPs or 
companies. These ASes are connected in specific ways, and traffic 
follows rules based on their relationships:

1. **Customer to Provider (C2P):** A smaller AS (customer) pays a 
bigger AS (provider) to use its network.
2. **Peer to Peer (P2P):** Two ASes exchange traffic for free, as equals.
3. **Provider to Customer (P2C):** A bigger AS sends traffic to a 
smaller AS (its customer).

To keep internet routing fair and efficient, the traffic paths between 
ASes should follow a specific order of these relationships. This is 
where the concept of **valley-free routing** comes in.


### What is "Valley-Free Routing"?

Imagine traffic flows through a "valley" when it:

1. Starts at the origin AS (the bottom-left of the valley),
2. Climbs up to a peak (top of the valley) by going through provider networks,
3. Then comes back down the other side (bottom-right) to reach the 
verifying AS by going through customer networks.

The key rule for a "valley-free" AS_PATH is:

- Traffic **cannot go up, down, then up again** (no valleys within a valley).
- It must follow a **logical pattern**: either up (Customer to Provider), 
across (Peer), or down (Provider to Customer), but never switch back to "up" 
again once it starts going "down."


### The "Ramps" and "Apexes"

The AS_PATH can be thought of as two ramps:

1. **Up-ramp (on the right):**
   - Starts from the **origin AS** (AS(1)).
   - Traffic climbs **up** through **Customer-to-Provider** relationships.
   - It keeps going until it reaches a **peak** (apex).

2. **Down-ramp (on the left):**
   - Starts from the **verifying AS** (AS(N+1), which is checking the path).
   - Traffic goes **down** through **Provider-to-Customer** relationships.
   - It also ends at the same **peak** (apex).


### What are the "Apexes"?

The **apex** is where the up-ramp and the down-ramp meet. This is the 
highest point of the path, where the relationship changes from "up" 
(customer-to-provider) to "down" (provider-to-customer).


### The Rule in Simple Terms

For the AS_PATH to be valid (valley-free), the up-ramp and the down-ramp 
must meet at the apex in one of two ways:

1. **No hops between the apexes:**
   - The last AS on the up-ramp (AS(K)) is the same as the first AS on 
   the down-ramp (AS(L)).
   - In this case, the up-ramp smoothly transitions into the down-ramp.

2. **Just one hop between the apexes:**
   - If there’s one AS between the last AS on the up-ramp (AS(K)) and the 
   first AS on the down-ramp (AS(L)).
   - This means AS(K) and AS(L) are **peers** (P2P relationship).


### Why Does This Matter?

If there are more than one hop between the apexes, it could indicate 
a **"valley" within the AS_PATH**, where the traffic has gone up, 
down, and up again, breaking the rules. Such a path is invalid because:

- It suggests an inefficient or manipulated route.
- It might not follow the business agreements between ASes (like a customer 
pretending to be a provider).

By ensuring the up-ramp and down-ramp meet cleanly (with 0 or 1 hop), 
we ensure the AS_PATH is logical, secure, and valid.


### Visualizing It

Imagine climbing up one side of a mountain (the up-ramp), then coming down the 
other side (the down-ramp). The "peak" of the mountain is the apex:

- If there’s no gap at the peak, you’re directly at the same point where the 
descent starts.
- If there’s one small step at the peak (a peer connection), that’s still fine.
- If there’s a large gap, it’s invalid because you’ve left the valley and 
climbed another mountain, which shouldn’t happen in a valley-free path.

## ASPA Based AS_PATH Verification


