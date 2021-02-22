#include "ClientInfo.h"

namespace pla::common::client_info {

ClientInfo::ClientInfo(const sf::IpAddress& ipAddr, const unsigned short& port)
  : m_Ip(ipAddr), m_Port(port)
  , m_state(ClientStates::Invalid)
{

}


const sf::IpAddress& ClientInfo::getIpAddress()
{
  return m_Ip;
}


const unsigned short& ClientInfo::getPort() const
{
  return m_Port;
}


ClientStates ClientInfo::getClientState() const {
  return m_state;
}


void ClientInfo::setClientState(const ClientStates& state) {
  m_state = state;
}


std::shared_ptr<sf::TcpSocket> &ClientInfo::getClientSocket()
{
  return m_socket;
}


void ClientInfo::setClientSocket(std::shared_ptr<sf::TcpSocket>& socket)
{
  m_socket = std::move(socket);
}

} // namespaces