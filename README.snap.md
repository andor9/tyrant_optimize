# tyrant-unleashed-optimizer snap usage

This is about using the snap'ed version of tyrant-unleashed-optimizer (tuo)

Working features:
 - updating local card database
 - simulating fights

Exposed commands:
 - tyrant-unleashed-optimizer.updatexml
 - tyrant-unleashed-optimizer.sim

Known issues:
  - To get to your card data files  the home plug needs to be connected manually
    after snap install. This is due to file access being a potential risk (and
    snaps are a lot about security and isolation). To connect the home plug
    call:
      "sudo snap connect tyrant-unleashed-optimizer:home ubuntu-core:home"

Card-Update:
  - Note: As usual if you don't have a "data" dir yet you have to create one via ``mkdir data``
  - Then to actually call it:
      ``tyrant-unleashed-optimizer.updatexml``

Simulate:
  - Note: as usual with tuo you have to run this where you have your ``data`` dir.
  - tyrant-unleashed-optimizer.sim "Lord Silus, Extreme Barrager #2" "Lord Silus, Atomic Wardriver #2" sim 100
