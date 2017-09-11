/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file WarpHostCapability.cpp
 */

#include "WarpHostCapability.h"
#include "BlockChain.h"
#include "SnapshotDownloader.h"

#include <boost/fiber/fiber.hpp>


using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

namespace
{

class WarpPeerObserver: public WarpPeerObserverFace
{
public:
	WarpPeerObserver(SnapshotDownloader& _downloader, 
		WarpHostCapability& _host,
		BlockChain const& _blockChain,
		boost::fibers::unbuffered_channel<bytes>& _manifestChannel/*,
		boost::fibers::unbuffered_channel<std::shared_ptr<WarpPeerCapability>>& _newConnectionsChannel*/): 
		m_downloader(_downloader), m_host(_host), m_blockChain(_blockChain), m_manifestChannel(_manifestChannel)/*, m_newConnectionsChannel(_newConnectionsChannel)*/ {}

	void onPeerStatus(shared_ptr<WarpPeerCapability> _peer) override
	{
//		m_downloader.onPeerStatus(_peer);
		if (_peer->validateStatus(m_blockChain.genesisHash(), {m_host.protocolVersion()}, m_host.networkId()))
			_peer->requestManifest();
	}

	void onPeerManifest(shared_ptr<WarpPeerCapability> _peer, RLP const& _r) override
	{
		//m_downloader.onPeerManifest(_peer, _r);
		m_manifestChannel.push(_r.toBytes());
	}

	void onPeerData(shared_ptr<WarpPeerCapability> _peer, RLP const& _r) override
	{
		m_downloader.onPeerData(_peer, _r);
	}

	void onPeerRequestTimeout(std::shared_ptr<WarpPeerCapability> _peer, Asking _asking) override
	{
		m_downloader.onPeerRequestTimeout(_peer, _asking);
	}

private:
	SnapshotDownloader& m_downloader;

	WarpHostCapability& m_host;
	BlockChain const& m_blockChain;
	boost::fibers::unbuffered_channel<bytes>& m_manifestChannel;
//		boost::fibers::unbuffered_channel<std::shared_ptr<WarpPeerCapability>>& m_newConnectionsChannel;
};

}

WarpHostCapability::WarpHostCapability(BlockChain const& _blockChain, u256 const& _networkId, boost::filesystem::path const& _snapshotPath):
	m_blockChain(_blockChain),
	m_networkId(_networkId),
	m_downloader(new SnapshotDownloader(*this, _blockChain, _snapshotPath)),
	m_peerObserver(make_shared<WarpPeerObserver>(*m_downloader, *this, m_blockChain, m_manifestChannel))
{
	boost::fibers::fiber f([this](){
		bytes manifestBytes;
		m_manifestChannel.pop(manifestBytes);

		RLP _r(manifestBytes); // TODO rename
		if (!_r.isList() || _r.itemCount() != 1)
			return;

		RLP const manifest = _r[0];

		u256 version = manifest[0].toInt<u256>();
		if (version != 2)
			return;

		u256 const blockNumber = manifest[4].toInt<u256>();
/* TODO
if (blockNumber != m_syncingSnapshotNumber)
			return;
*/
		h256s const stateHashes = manifest[1].toVector<h256>();
		h256s const blockHashes = manifest[2].toVector<h256>();

		h256 const stateRoot = manifest[3].toHash<h256>();
		h256 const blockHash = manifest[5].toHash<h256>();

		clog(SnapshotLog) << "MANIFEST: "
			<< "version " << version
			<< "state root " << stateRoot
			<< "block number " << blockNumber
			<< "block hash " << blockHash;


	});
}

shared_ptr<Capability> WarpHostCapability::newPeerCapability(shared_ptr<SessionFace> const& _s, unsigned _idOffset, p2p::CapDesc const& _cap, uint16_t _capID)
{
	auto ret = HostCapability<WarpPeerCapability>::newPeerCapability(_s, _idOffset, _cap, _capID);

	auto cap = capabilityFromSession<WarpPeerCapability>(*_s, _cap.second);
	assert(cap);
	cap->init(
		c_WarpProtocolVersion,
		m_networkId,
		m_blockChain.details().totalDifficulty,
		m_blockChain.currentHash(),
		m_blockChain.genesisHash(),
		m_peerObserver
	);

	return ret;
}

void WarpHostCapability::doWork()
{
	// TODO call tick() on peers similar to EthereumHost
}

