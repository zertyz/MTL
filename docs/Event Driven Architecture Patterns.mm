<map version="freeplane 1.8.0">
<!--To view this file, download free mind mapping software Freeplane from http://freeplane.sourceforge.net -->
<node TEXT="Event Driven Architecture Patterns" FOLDED="false" ID="ID_191153586" CREATED="1585335282773" MODIFIED="1585335317875" ICON_SIZE="36.0 pt" STYLE="oval">
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
<node TEXT="Event Carried State Transfer" POSITION="right" ID="ID_273436263" CREATED="1585335282776" MODIFIED="1585336702121" HGAP_QUANTITY="91.26829627750192 pt" VSHIFT_QUANTITY="-30.731708746733723 pt">
<edge COLOR="#007c7c"/>
<richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Instead of requiring that listeners go back to acquire more information about the event, all the information possibly needed is carried along with the event
    </p>
  </body>
</html>

</richcontent>
<node TEXT="Bright and Dark sides" ID="ID_843172984" CREATED="1585336451410" MODIFIED="1585336457300">
<node TEXT="Distributed DB" ID="ID_997949848" CREATED="1585336459853" MODIFIED="1585336518803"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Every subscriber may use that info to keep their own state of changes -- there own database
    </p>
  </body>
</html>
</richcontent>
</node>
<node TEXT="Eventual Consistency" ID="ID_319372310" CREATED="1585336466720" MODIFIED="1585337357454"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Comes along with a shared copy of the data -- which may be synced at a later time. Do the event creator need to keep it consolidated or it may just keep the logs?
    </p>
  </body>
</html>

</richcontent>
</node>
</node>
</node>
<node TEXT="Event Sourcing" POSITION="right" ID="ID_495676614" CREATED="1585335282777" MODIFIED="1585336748049" HGAP_QUANTITY="101.80488213352491 pt" VSHIFT_QUANTITY="-42.14634342409195 pt">
<edge COLOR="#009900"/>
<node TEXT="Explanation" ID="ID_744391648" CREATED="1585335282777" MODIFIED="1585336839321" HGAP_QUANTITY="15.756097642670499 pt" VSHIFT_QUANTITY="-14.926829962699236 pt"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      The events generated are able to be stored as replayable logs
    </p>
  </body>
</html>

</richcontent>
</node>
<node TEXT="Examples" ID="ID_903669612" CREATED="1585335282777" MODIFIED="1585336846593">
<node TEXT="Version Control" ID="ID_1376885863" CREATED="1585336847916" MODIFIED="1585337022016"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      You may cherry-pick some commits and replay from there on to regenerate your source code; You may merge with other commits.
    </p>
  </body>
</html>

</richcontent>
</node>
<node TEXT="InstantVAS&apos;s SMSes messages log table" ID="ID_1970794910" CREATED="1585336852873" MODIFIED="1585337018339"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Reprocessing the log of messages we may, with restrictions, rebuild all the rest of the database. Restrictions: timeouts must be taken into account; eventual improvements on the messages recognition patterns will inevitably affect the outcome.
    </p>
  </body>
</html>

</richcontent>
</node>
</node>
<node TEXT="Bright and Dark sides" ID="ID_397216611" CREATED="1585336451410" MODIFIED="1585336457300">
<node TEXT="Inconsistent State" ID="ID_188078244" CREATED="1585336466720" MODIFIED="1585337281535"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Not every body may agree with the replay outcome. To mitigate this, some use snapshots: from time to time, the state is consolidated and considered as &quot;the true fact&quot; -- just like done in a bank account. From that point on, new changes start to build up from there.
    </p>
  </body>
</html>

</richcontent>
</node>
<node TEXT="Interaction with other systems" ID="ID_1558203207" CREATED="1585337413752" MODIFIED="1585337643381"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Care must be taken with interactions with other systems... on the replay, they won't be able to be called again. To resolve that, their requests/responses may be saved as well. This may be impossible to solve, 'tough: imagine you have an algorithmic trading system and want to test a faster algorithm: the market events happened in response to your slow algorithm and, almost certainly, in this case, a faster response alters the chain of cause and effect.
    </p>
  </body>
</html>

</richcontent>
</node>
</node>
<node TEXT="CQRS" ID="ID_1342659867" CREATED="1585340656191" MODIFIED="1585340661107">
<node TEXT="Common Query Responsability Segregation" ID="ID_1653679868" CREATED="1585340663217" MODIFIED="1585340698137"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Queries &amp; Updates are segregated -- separated components
    </p>
  </body>
</html>

</richcontent>
<node TEXT="Critics" ID="ID_1059403649" CREATED="1585340717852" MODIFIED="1585340752902"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      This seems to be used just to control the connections writing and connections querying... ?
    </p>
  </body>
</html>

</richcontent>
</node>
</node>
</node>
<node TEXT="References" ID="ID_1436889767" CREATED="1585337677200" MODIFIED="1585337683381">
<node TEXT="Creg Young" ID="ID_1337351032" CREATED="1585337686003" MODIFIED="1585337690891"/>
</node>
</node>
<node TEXT="Orchestrator" POSITION="right" ID="ID_90823870" CREATED="1585335282777" MODIFIED="1585335481907" HGAP_QUANTITY="56.146343424091945 pt" VSHIFT_QUANTITY="-61.46341749346744 pt">
<edge COLOR="#ff0000"/>
<node TEXT="Explanation" ID="ID_1201198400" CREATED="1585335282777" MODIFIED="1585335799113" HGAP_QUANTITY="15.756097642670499 pt" VSHIFT_QUANTITY="-15.804878784034486 pt"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Event Driven Systems usually should react to single events, when they happen. Not always this is possible -- for instance, one may accumulate the operations to be sent to a database and then they will be sent at a later time.
    </p>
  </body>
</html>

</richcontent>
</node>
<node TEXT="Examples" ID="ID_1615683347" CREATED="1585335282777" MODIFIED="1585335517341" HGAP_QUANTITY="17.512195285340997 pt" VSHIFT_QUANTITY="3.512195285340997 pt">
<node TEXT="Batch Processing" ID="ID_487743057" CREATED="1585335522084" MODIFIED="1585335700186"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      The event processor receives the command of processing some data. It, then, reads the database for more details, process de data and save the results (on the database). Scalability is obtained by increasing the command processors (event consumers) and careful control on the number of simultaneous resources accesses (the database, in this case) must be taken care of -- usually, it can be limited to n per instance (most of the times n is just 1)
    </p>
  </body>
</html>

</richcontent>
</node>
</node>
</node>
<node POSITION="left" ID="ID_25434398" CREATED="1585335282777" MODIFIED="1585335461558" HGAP_QUANTITY="73.70731985079694 pt" VSHIFT_QUANTITY="-37.75609931741573 pt"><richcontent TYPE="NODE">

<html>
  <head>
    
  </head>
  <body>
    <p>
      <b>Notification</b>&nbsp;/ <b>Command</b>
    </p>
  </body>
</html>

</richcontent>
<edge COLOR="#cc00ff"/>
<node TEXT="Events" ID="ID_352937383" CREATED="1585335282777" MODIFIED="1585336012068" HGAP_QUANTITY="13.121951178664752 pt" VSHIFT_QUANTITY="-9.658537034687741 pt"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Usually represent a fact (something that already happened) and may be issued to give others a chance to do something about it. They should be named with that in mind and, also, they tend to be of interest of <b><i>listeners</i></b>.
    </p>
  </body>
</html>

</richcontent>
<node TEXT="Examples" ID="ID_663795888" CREATED="1585335915940" MODIFIED="1585335920664">
<node TEXT="Customer Management Microservice" ID="ID_1221053689" CREATED="1585336023696" MODIFIED="1585336105456"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      The microservice may issue events such as &quot;NEW_USER_ADDED&quot; or &quot;CUSTOMER_ADDRESS_CHANGED_BY_CUSTOMER&quot;
    </p>
  </body>
</html>

</richcontent>
</node>
</node>
</node>
<node TEXT="Command" ID="ID_966834988" CREATED="1585335282777" MODIFIED="1585337862675" HGAP_QUANTITY="15.756097642670499 pt" VSHIFT_QUANTITY="-7.902439392017243 pt"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      Represents the request for processing or request for an answer. Should be named accordingly and, usually, are subscribed by <b><i>consumers</i></b>. For sure, listeners might also be interested into it.
    </p>
  </body>
</html>

</richcontent>
<node TEXT="Examples" ID="ID_611503725" CREATED="1585337864227" MODIFIED="1585337866745">
<node TEXT="HTTP Server" ID="ID_171020409" CREATED="1585337868018" MODIFIED="1585337915773"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      The request for &quot;GET_PAGE_X&quot; must be consumed and may be listened, for instance, by the log sub-system.
    </p>
  </body>
</html>

</richcontent>
</node>
</node>
</node>
<node TEXT="Bright and Dark sides" ID="ID_1143479509" CREATED="1585336132831" MODIFIED="1585336191716">
<node TEXT="&quot;How is this being used&quot; syndrom" ID="ID_1921939770" CREATED="1585336194736" MODIFIED="1585336263962"><richcontent TYPE="DETAILS">

<html>
  <head>
    
  </head>
  <body>
    <p>
      One may easily loose track, when going through the code, of the systems that listen to our event. This may be mitigated by requiring every listener / consumer to provide their names.
    </p>
  </body>
</html>

</richcontent>
</node>
</node>
</node>
<node TEXT="Anti-Patterns" POSITION="left" ID="ID_1822402846" CREATED="1585335282777" MODIFIED="1585578301775" HGAP_QUANTITY="84.24390570681993 pt" VSHIFT_QUANTITY="-36.00000167474521 pt">
<edge COLOR="#ff9900"/>
<node TEXT="IDEA 5.1" ID="ID_1274507163" CREATED="1585335282777" MODIFIED="1585335282777" HGAP_QUANTITY="14.0 pt" VSHIFT_QUANTITY="-21.07317171204598 pt"/>
<node TEXT="IDEA 5.2" ID="ID_1203076641" CREATED="1585335282778" MODIFIED="1585335282778"/>
</node>
<node TEXT="References" POSITION="left" ID="ID_575079876" CREATED="1585335282778" MODIFIED="1585578294484" HGAP_QUANTITY="67.56097810145019 pt" VSHIFT_QUANTITY="-42.14634342409195 pt">
<edge COLOR="#0000ff"/>
<node TEXT="Martin Fawler" ID="ID_317662625" CREATED="1585335282778" MODIFIED="1585335342558" HGAP_QUANTITY="14.878048821335248 pt" VSHIFT_QUANTITY="-18.439025248040235 pt">
<node TEXT="youtube" ID="ID_681292676" CREATED="1585335350193" MODIFIED="1585335353326"/>
<node TEXT="website" ID="ID_878237485" CREATED="1585335354623" MODIFIED="1585335357195"/>
</node>
<node TEXT="IDEA 4.2" ID="ID_1522387997" CREATED="1585335282778" MODIFIED="1585335282778"/>
</node>
</node>
</map>
