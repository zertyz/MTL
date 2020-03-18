<map version="freeplane 1.8.0">
<!--To view this file, download free mind mapping software Freeplane from http://freeplane.sourceforge.net -->
<node TEXT="Event Model" FOLDED="false" ID="ID_191153586" CREATED="1584490898011" MODIFIED="1584498288643" ICON_SIZE="36.0 pt" STYLE="oval">
<font SIZE="22"/>
<hook NAME="MapStyle">
    <properties fit_to_viewport="false" edgeColorConfiguration="#808080ff,#ff0000ff,#0000ffff,#00ff00ff,#ff00ffff,#00ffffff,#7c0000ff,#00007cff,#007c00ff,#7c007cff,#007c7cff,#7c7c00ff"/>

<map_styles>
<stylenode LOCALIZED_TEXT="styles.root_node" STYLE="oval" UNIFORM_SHAPE="true" VGAP_QUANTITY="24.0 pt">
<font SIZE="24"/>
<stylenode LOCALIZED_TEXT="styles.predefined" POSITION="right" STYLE="bubble">
<stylenode LOCALIZED_TEXT="default" ICON_SIZE="12.0 pt" COLOR="#000000" STYLE="fork">
<font NAME="SansSerif" SIZE="10" BOLD="false" ITALIC="false"/>
</stylenode>
<stylenode LOCALIZED_TEXT="defaultstyle.details"/>
<stylenode LOCALIZED_TEXT="defaultstyle.attributes">
<font SIZE="9"/>
</stylenode>
<stylenode LOCALIZED_TEXT="defaultstyle.note" COLOR="#000000" BACKGROUND_COLOR="#ffffff" TEXT_ALIGN="LEFT"/>
<stylenode LOCALIZED_TEXT="defaultstyle.floating">
<edge STYLE="hide_edge"/>
<cloud COLOR="#f0f0f0" SHAPE="ROUND_RECT"/>
</stylenode>
</stylenode>
<stylenode LOCALIZED_TEXT="styles.user-defined" POSITION="right" STYLE="bubble">
<stylenode LOCALIZED_TEXT="styles.topic" COLOR="#18898b" STYLE="fork">
<font NAME="Liberation Sans" SIZE="10" BOLD="true"/>
</stylenode>
<stylenode LOCALIZED_TEXT="styles.subtopic" COLOR="#cc3300" STYLE="fork">
<font NAME="Liberation Sans" SIZE="10" BOLD="true"/>
</stylenode>
<stylenode LOCALIZED_TEXT="styles.subsubtopic" COLOR="#669900">
<font NAME="Liberation Sans" SIZE="10" BOLD="true"/>
</stylenode>
<stylenode LOCALIZED_TEXT="styles.important">
<icon BUILTIN="yes"/>
</stylenode>
</stylenode>
<stylenode LOCALIZED_TEXT="styles.AutomaticLayout" POSITION="right" STYLE="bubble">
<stylenode LOCALIZED_TEXT="AutomaticLayout.level.root" ICON_SIZE="64.0 pt" COLOR="#000000" STYLE="oval" SHAPE_HORIZONTAL_MARGIN="10.0 pt" SHAPE_VERTICAL_MARGIN="10.0 pt">
<font NAME="Segoe Print" SIZE="22"/>
<edge COLOR="#ffffff"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,1" ICON_SIZE="32.0 px" COLOR="#000000" BACKGROUND_COLOR="#ffffcc" STYLE="bubble" SHAPE_HORIZONTAL_MARGIN="2.6 pt" SHAPE_VERTICAL_MARGIN="2.6 pt" BORDER_WIDTH_LIKE_EDGE="true">
<font SIZE="18" BOLD="false" ITALIC="true"/>
<edge STYLE="sharp_bezier" WIDTH="8"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,2" ICON_SIZE="28.0 px" COLOR="#000000" BORDER_WIDTH_LIKE_EDGE="true">
<font SIZE="16"/>
<edge STYLE="bezier" WIDTH="4"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,3" ICON_SIZE="24.0 px" COLOR="#000000" BORDER_WIDTH_LIKE_EDGE="true">
<font SIZE="14"/>
<edge STYLE="bezier" WIDTH="3"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,4" ICON_SIZE="24.0 px" COLOR="#111111" BORDER_WIDTH_LIKE_EDGE="true">
<font SIZE="13"/>
<edge STYLE="bezier" WIDTH="2"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,5" ICON_SIZE="24.0 px" BORDER_WIDTH_LIKE_EDGE="true">
<font SIZE="12"/>
<edge STYLE="bezier" WIDTH="1"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,6" ICON_SIZE="24.0 px">
<edge STYLE="bezier"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,7" ICON_SIZE="16.0 px">
<font SIZE="10"/>
<edge STYLE="bezier"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,8" ICON_SIZE="16.0 px">
<edge STYLE="bezier"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,9" ICON_SIZE="16.0 px">
<edge STYLE="bezier"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,10" ICON_SIZE="16.0 px">
<edge STYLE="bezier"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,11" ICON_SIZE="16.0 px">
<edge STYLE="bezier"/>
</stylenode>
</stylenode>
</stylenode>
</map_styles>
</hook>
<hook NAME="AutomaticEdgeColor" COUNTER="15" RULE="ON_BRANCH_CREATION"/>
<hook NAME="accessories/plugins/AutomaticLayout.properties" VALUE="ALL"/>
<edge COLOR="#ffffff"/>
<node TEXT="CommunicationPatterns" POSITION="right" ID="ID_273436263" CREATED="1584490898039" MODIFIED="1584498689854" HGAP_QUANTITY="91.26829627750192 pt" VSHIFT_QUANTITY="-30.731708746733723 pt">
<edge COLOR="#007c7c"/>
<node TEXT="Producer(s) / Consumer(s)" ID="ID_588194298" CREATED="1584490898041" MODIFIED="1584498626226" HGAP_QUANTITY="15.756097642670499 pt" VSHIFT_QUANTITY="-12.292683498693489 pt"/>
<node TEXT="Broadcast" ID="ID_1104103844" CREATED="1584490898041" MODIFIED="1584498593816"/>
<node TEXT="RPC" ID="ID_849688082" CREATED="1584498594625" MODIFIED="1584498599218"/>
<node ID="ID_734055308" CREATED="1584499817834" MODIFIED="1584499831055"><richcontent TYPE="NODE">

<html>
  <head>
    
  </head>
  <body>
    <p>
      <b>... or any combination of those</b>
    </p>
  </body>
</html>

</richcontent>
</node>
</node>
<node TEXT="EventProcessors" POSITION="right" ID="ID_495676614" CREATED="1584490898042" MODIFIED="1584498674576" HGAP_QUANTITY="101.80488213352491 pt" VSHIFT_QUANTITY="-42.14634342409195 pt">
<edge COLOR="#009900"/>
<node TEXT="Thread Configurations" ID="ID_744391648" CREATED="1584490898043" MODIFIED="1584498863175" HGAP_QUANTITY="15.756097642670499 pt" VSHIFT_QUANTITY="-14.926829962699236 pt">
<node TEXT="1 Thread, 1 Consumer" ID="ID_787729860" CREATED="1584498871933" MODIFIED="1584498895703"/>
<node TEXT="n Threads, n Consumers" ID="ID_491034772" CREATED="1584498897361" MODIFIED="1584498911067"/>
<node TEXT="1 Thread, 1 Listener" ID="ID_1254851143" CREATED="1584498916916" MODIFIED="1584498957335"/>
<node TEXT="1 Thread, n Listeners" ID="ID_897961463" CREATED="1584498958561" MODIFIED="1584498976869"/>
<node TEXT="n Threads, 1..n Listeners per thread" ID="ID_47426854" CREATED="1584498977462" MODIFIED="1584499036885"/>
<node TEXT="1..n Threads, 1 Consumer + 1..n Listeners per thread" ID="ID_492294709" CREATED="1584499000943" MODIFIED="1584499103569"/>
</node>
<node TEXT="Responsabilities" ID="ID_1918235917" CREATED="1584500044651" MODIFIED="1584500071576">
<node TEXT="Create &amp; manage all threads to process the given event(s)" ID="ID_526652300" CREATED="1584500073862" MODIFIED="1584500407724"/>
<node TEXT="Provide the loop algorithm for each one of them" ID="ID_672614337" CREATED="1584500098856" MODIFIED="1584500138050"/>
<node ID="ID_1231344032" CREATED="1584500417484" MODIFIED="1584500528540"><richcontent TYPE="NODE">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Provide the optimal <b>Scheduling Algorithms</b>&nbsp;for each thread:
    </p>
  </body>
</html>

</richcontent>
</node>
<node ID="ID_1281624515" CREATED="1584500153734" MODIFIED="1584501314875"><richcontent TYPE="NODE">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Respecting all possible combinations of <b>EventInfo</b>&nbsp;&amp; <b>CommunicationPatterns</b>
    </p>
  </body>
</html>

</richcontent>
</node>
<node ID="ID_466972396" CREATED="1584500233810" MODIFIED="1584500545921"><richcontent TYPE="NODE">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Providing specific algorithms for each <b>EventChannel</b>
    </p>
  </body>
</html>

</richcontent>
</node>
<node ID="ID_1769639577" CREATED="1584500883072" MODIFIED="1584501448677"><richcontent TYPE="NODE">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Using the requested <b>SpinOperation</b>&nbsp;for the Event's Request &amp; Answer loops
    </p>
  </body>
</html>

</richcontent>
</node>
</node>
<node TEXT="Scheduling Algorithms" ID="ID_903669612" CREATED="1584490898043" MODIFIED="1584498840467">
<node TEXT="for threads with:" ID="ID_1497706912" CREATED="1584499872202" MODIFIED="1584499893856">
<node TEXT="a single event &amp;" ID="ID_1244387299" CREATED="1584500606253" MODIFIED="1584500676716">
<node TEXT="a single consumer" ID="ID_903926793" CREATED="1584499896234" MODIFIED="1584499906676"/>
<node TEXT="a single listener" ID="ID_77814067" CREATED="1584499907270" MODIFIED="1584499914581"/>
<node TEXT="several listeners" ID="ID_1825166724" CREATED="1584499915731" MODIFIED="1584499921248"/>
<node TEXT="a consumer + 1 listener" ID="ID_1352963256" CREATED="1584499945168" MODIFIED="1584499967399"/>
<node TEXT="a consumer + several listeners" ID="ID_197167754" CREATED="1584499968416" MODIFIED="1584499976795"/>
</node>
<node TEXT="more than 1 event &amp;" ID="ID_1503796362" CREATED="1584500633018" MODIFIED="1584500683138">
<node TEXT="just consumers" ID="ID_1841668172" CREATED="1584500728508" MODIFIED="1584500757910"/>
<node TEXT="just listeners" ID="ID_734055190" CREATED="1584500759969" MODIFIED="1584500766419"/>
<node TEXT="consumers + listeners" ID="ID_216309111" CREATED="1584500777492" MODIFIED="1584500791386"/>
</node>
</node>
</node>
</node>
<node TEXT="EventChannels" POSITION="right" ID="ID_90823870" CREATED="1584490898044" MODIFIED="1584498712417" HGAP_QUANTITY="56.146343424091945 pt" VSHIFT_QUANTITY="-61.46341749346744 pt">
<edge COLOR="#ff0000"/>
<node TEXT="DirectEventChannel" ID="ID_1201198400" CREATED="1584490898045" MODIFIED="1584498725796" HGAP_QUANTITY="15.756097642670499 pt" VSHIFT_QUANTITY="-15.804878784034486 pt"/>
<node TEXT="QueueEventChannel" ID="ID_1615683347" CREATED="1584490898045" MODIFIED="1584498750833" HGAP_QUANTITY="17.512195285340997 pt" VSHIFT_QUANTITY="3.512195285340997 pt">
<node TEXT="Queues" ID="ID_1950658754" CREATED="1584490898046" MODIFIED="1584499159481" HGAP_QUANTITY="73.70731985079694 pt" VSHIFT_QUANTITY="-37.75609931741573 pt">
<node TEXT="PointerQueue" ID="ID_1864576214" CREATED="1584490898047" MODIFIED="1584498333933" HGAP_QUANTITY="13.121951178664752 pt" VSHIFT_QUANTITY="-9.658537034687741 pt">
<node TEXT="Allocators" ID="ID_3683886" CREATED="1584490898049" MODIFIED="1584499187908" HGAP_QUANTITY="84.24390570681993 pt" VSHIFT_QUANTITY="-36.00000167474521 pt">
<node TEXT="Pool Allocator" ID="ID_1679572713" CREATED="1584490898049" MODIFIED="1584498372277" HGAP_QUANTITY="14.0 pt" VSHIFT_QUANTITY="-21.07317171204598 pt"/>
</node>
</node>
<node TEXT="RingBufferQueue" ID="ID_80153119" CREATED="1584490898048" MODIFIED="1584498350314" HGAP_QUANTITY="15.756097642670499 pt" VSHIFT_QUANTITY="-7.902439392017243 pt"/>
</node>
</node>
<node TEXT="DistributedEventChannel" ID="ID_820855915" CREATED="1584498779711" MODIFIED="1584498793142">
<node TEXT="ZeroMQ based?" ID="ID_1564766141" CREATED="1584498797840" MODIFIED="1584498812179"/>
</node>
</node>
<node TEXT="EventInfo" POSITION="left" ID="ID_25434398" CREATED="1584490898046" MODIFIED="1584499267563" HGAP_QUANTITY="73.70731985079694 pt" VSHIFT_QUANTITY="-37.75609931741573 pt">
<edge COLOR="#cc00ff"/>
<node TEXT="RequestInfo" ID="ID_352937383" CREATED="1584490898047" MODIFIED="1584499294837" HGAP_QUANTITY="13.121951178664752 pt" VSHIFT_QUANTITY="-9.658537034687741 pt">
<node TEXT="all infos related to the event" ID="ID_668519299" CREATED="1584499542050" MODIFIED="1584499564258"/>
</node>
<node TEXT="AnswerInfo" ID="ID_966834988" CREATED="1584490898048" MODIFIED="1584499302406" HGAP_QUANTITY="15.756097642670499 pt" VSHIFT_QUANTITY="-7.902439392017243 pt">
<node TEXT="the results after the RPC consumers process the &apos;RequestInfo&apos;" ID="ID_1462242212" CREATED="1584499572461" MODIFIED="1584499613897"/>
</node>
<node TEXT="CommunicationsInfo" ID="ID_1527554640" CREATED="1584499303630" MODIFIED="1584499729186">
<node TEXT="Control data to allow the synchronization of Pub/Sub, Broadcast, RPC &amp; the several possible thread configurations of the &apos;EventProcessor&apos;" ID="ID_1414875632" CREATED="1584499622839" MODIFIED="1584499740539"/>
</node>
<node TEXT="DebugInfo" ID="ID_1497021075" CREATED="1584499331640" MODIFIED="1584499334608">
<node TEXT="&lt;&lt;get from the exercise&gt;&gt;" ID="ID_16074375" CREATED="1584499753681" MODIFIED="1584499795469"/>
</node>
<node TEXT="MetricsInfo" ID="ID_133500827" CREATED="1584499335865" MODIFIED="1584499345412">
<node TEXT="&lt;&lt;get from the exercise&gt;&gt;" ID="ID_918246848" CREATED="1584499763627" MODIFIED="1584499773639"/>
</node>
<node TEXT="Layouts" ID="ID_546298409" CREATED="1584499358556" MODIFIED="1584499368026">
<font BOLD="true"/>
<node TEXT="All on the same struct" ID="ID_362266931" CREATED="1584499374185" MODIFIED="1584499385365">
<node TEXT="aligned or not" ID="ID_144343892" CREATED="1584499448605" MODIFIED="1584499458957"/>
</node>
<node TEXT="Each on their own struct" ID="ID_1137067935" CREATED="1584499387248" MODIFIED="1584499412628">
<node TEXT="may each one be aligned?" ID="ID_865832426" CREATED="1584499510466" MODIFIED="1584499524737"/>
</node>
</node>
</node>
<node TEXT="SpinOperation" POSITION="left" ID="ID_1822402846" CREATED="1584490898049" MODIFIED="1584500913251" HGAP_QUANTITY="84.24390570681993 pt" VSHIFT_QUANTITY="-36.00000167474521 pt">
<edge COLOR="#ff9900"/>
<node TEXT="Mutex" ID="ID_1274507163" CREATED="1584490898049" MODIFIED="1584500921976" HGAP_QUANTITY="14.0 pt" VSHIFT_QUANTITY="-21.07317171204598 pt"/>
<node TEXT="Futex" ID="ID_422155931" CREATED="1584500922643" MODIFIED="1584500925046"/>
<node TEXT="Spin + Mutex|Futex fallback" ID="ID_424737904" CREATED="1584500927343" MODIFIED="1584500952899"/>
<node TEXT="Spin" ID="ID_1648814275" CREATED="1584500958272" MODIFIED="1584500960435"/>
</node>
<node TEXT="...." POSITION="left" ID="ID_575079876" CREATED="1584490898050" MODIFIED="1584499238263" HGAP_QUANTITY="67.56097810145019 pt" VSHIFT_QUANTITY="-42.14634342409195 pt">
<edge COLOR="#0000ff"/>
<node TEXT="IDEA 4.1" ID="ID_317662625" CREATED="1584490898051" MODIFIED="1584490898051" HGAP_QUANTITY="14.878048821335248 pt" VSHIFT_QUANTITY="-18.439025248040235 pt"/>
<node TEXT="IDEA 4.2" ID="ID_1522387997" CREATED="1584490898052" MODIFIED="1584490898052"/>
</node>
</node>
</map>
